package net.floodlightcontroller.trafficmonitor;

import java.lang.Thread.State;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Map.Entry;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.core.internal.IOFSwitchService;
import net.floodlightcontroller.core.types.NodePortTuple;
import net.floodlightcontroller.linkdiscovery.ILinkDiscoveryService;
import net.floodlightcontroller.threadpool.IThreadPoolService;

import org.projectfloodlight.openflow.protocol.OFFactories;
import org.projectfloodlight.openflow.protocol.OFFactory;
import org.projectfloodlight.openflow.protocol.OFPortStatsEntry;
import org.projectfloodlight.openflow.protocol.OFPortStatsReply;
import org.projectfloodlight.openflow.protocol.OFStatsReply;
import org.projectfloodlight.openflow.protocol.OFStatsRequest;
import org.projectfloodlight.openflow.protocol.OFStatsType;
import org.projectfloodlight.openflow.protocol.OFVersion;
import org.projectfloodlight.openflow.protocol.match.Match;
import org.projectfloodlight.openflow.types.DatapathId;
import org.projectfloodlight.openflow.types.OFGroup;
import org.projectfloodlight.openflow.types.OFPort;
import org.projectfloodlight.openflow.types.TableId;
import org.projectfloodlight.openflow.types.U64;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.util.concurrent.ListenableFuture;

public class TrafficCollector {
	protected static final Logger logger = LoggerFactory.getLogger(TrafficCollector.class);
	
	private  IOFSwitchService 				switchService;
	private  IThreadPoolService 			threadPoolService;	// �̳߳�
	private  ILinkDiscoveryService			linkDiscoveryService;
	public static long portStatsInterval = 10;					// �ռ��������˿�ͳ����������,��λΪ��

	private static ScheduledFuture<?> portStatsCollector;		// ���ڽ����̳߳صķ���ֵ���ռ�port_stats
	
	public static HashMap<NodePortTuple, SwitchPortStatistics> prePortStatsBuffer = new HashMap<NodePortTuple, SwitchPortStatistics>(); // ���ڻ�����ǰ�Ľ������˿�ͳ����Ϣ		
	public static HashMap<NodePortTuple, SwitchPortStatistics> portStatsBuffer = new HashMap<NodePortTuple, SwitchPortStatistics>();	// ���ڻ��浱ǰ�Ľ������˿�ͳ����Ϣ
	public static HashMap<NodePortTuple, SwitchPortStatistics> abnormalTraffic = new HashMap<NodePortTuple, SwitchPortStatistics>();   	// �洢�쳣����
	public static HashMap<NodePortTuple, Date> 				   addFlowEntriesHistory = new HashMap<NodePortTuple, Date>();				// ��������Ӽ�¼����ֹ�·��ظ�������
	
	private  Policy policy;
	private  LinkedList<Event>	events;					
	
	private boolean isFirstTime2CollectSwitchStatistics = true;
	
	/**
	 * �̣߳����������ռ��������˿�ͳ����Ϣ������˿ڽ������ʣ���������
	 * ��startPortStatsCollection()��ʹ��
	 */
	protected class PortStatsCollector implements Runnable{
		@Override
		public void run() {

			Map<DatapathId, List<OFStatsReply>> replies = getSwitchStatistics(switchService.getAllSwitchDpids(), OFStatsType.PORT);
			if(!replies.isEmpty()){
				logger.info("Got port_stats_replies");
				
				/* ��һ���ռ�ͳ����Ϣ */
				if(isFirstTime2CollectSwitchStatistics){
					isFirstTime2CollectSwitchStatistics = false;
				
					/* ��¼�ռ���ͳ����Ϣ��prePortStatsReplies */
					savePortStatsReplies(prePortStatsBuffer, replies);
				}
				else{	/* ��ǰ�Ѿ��ռ�����һ��ͳ����Ϣ */
					savePortStatsReplies(portStatsBuffer, replies);
					
					if(prePortStatsBuffer!=null)
					for(Entry<NodePortTuple, SwitchPortStatistics> entry : prePortStatsBuffer.entrySet()){
						NodePortTuple npt = entry.getKey();
						
						/* ����˿ڽ������ʺͷ������ʲ����¶˿�ͳ����Ϣ */
						if( portStatsBuffer.containsKey(npt)){
							U64 rxBytes = portStatsBuffer.get(npt).getRxBytes().subtract(prePortStatsBuffer.get(npt).getRxBytes());
							U64 txBytes = portStatsBuffer.get(npt).getTxBytes().subtract(prePortStatsBuffer.get(npt).getTxBytes());
							
							long period = portStatsBuffer.get(npt).getDuration() - prePortStatsBuffer.get(npt).getDuration();
							
							U64 rxSpeed = U64.ofRaw(rxBytes.getValue() / period);
							U64 txSpeed = U64.ofRaw(txBytes.getValue() / period);
											
							/* ���� */
							portStatsBuffer.get(npt).setRxSpeed(rxSpeed);
							portStatsBuffer.get(npt).setTxSpeed(txSpeed);
						}					
					}
					
					/* ��ӡ�˿�ͳ����Ϣ 
					logger.info("ready to print stats");
					for(Entry<NodePortTuple, SwitchPortStatistics> e : portStatsBuffer.entrySet()){
						e.getValue().printPortStatistics();
					}
					*/
					
					/* ����prePortStatsBuffer */
					prePortStatsBuffer.clear();
					prePortStatsBuffer.putAll(portStatsBuffer);
					portStatsBuffer.clear();
					logger.info("prePortStatsBuffer updated");
					
					/* �˿����ʷ��� */
					abnormalTraffic.clear();
					TrafficAnalyzer.Analysis(prePortStatsBuffer, abnormalTraffic, policy);
					if(!abnormalTraffic.isEmpty()){
						TrafficController.executePolicy(switchService, linkDiscoveryService, abnormalTraffic, addFlowEntriesHistory, policy, events);
					}
					
				}
			}
		}
		
		/**
		 *  ���յ���port_stats_reply���浽port_stats_buffer��
		 * @param portStatsBuffer �洢ͳ����Ϣ�Ļ�����
		 * @param replies stats_reply��Ϣ�б�
		 */
		private void savePortStatsReplies
			(HashMap<NodePortTuple, SwitchPortStatistics> portStatsBuffer, Map<DatapathId, List<OFStatsReply>> replies){
			for( Entry<DatapathId, List<OFStatsReply>> entry : replies.entrySet()){
				/* ����port_stats_reply��Ϣ��������ֶ�ת�浽SwitchPortStatistics����, */
				OFPortStatsReply psr = (OFPortStatsReply)entry.getValue().get(0);
				for(OFPortStatsEntry e :  psr.getEntries()){
					NodePortTuple npt = new NodePortTuple(entry.getKey(), e.getPortNo());
					SwitchPortStatistics sps = new SwitchPortStatistics();
					
					sps.setDpid(npt.getNodeId());
					sps.setPortNo(npt.getPortId());
					sps.setRxBytes(e.getRxBytes());
					sps.setTxBytes(e.getTxBytes());
					sps.setDurationSec(e.getDurationSec());
					sps.setDurationNsec(e.getDurationNsec());
					
					sps.setRxPackets(e.getRxPackets());
					sps.setTxPackets(e.getTxPackets());
					sps.setRxDropped(e.getRxDropped());
					sps.setTxDropped(e.getTxDropped());
					
					/* ��������ֶ� */
					// ...
					
					/* ���潻����id�˿ںŵĶ�Ԫ��Ͷ˿�ͳ����Ϣ�������� */
					portStatsBuffer.put(npt, sps);
				}	
			}
		}
	
	}
	
	/**
	 * 	�����̳߳ط��񣬴����߳�������ִ��PortStatsCollector���е�run()��portStatsInterval������ִ������
	 */
	public void startPortStatsCollection(IOFSwitchService switchService, IThreadPoolService threadPoolService, ILinkDiscoveryService linkDiscoveryService, Policy policy, LinkedList<Event> events){		
		/* ��TrafficMonitor���õ�ʵ������Ӧ�����Խ��г�ʼ��  */
		this.switchService = switchService;
		this.threadPoolService = threadPoolService;
		this.linkDiscoveryService = linkDiscoveryService;
		this.policy = policy;
		this.events = events;
		
		/* �����̳߳ط����������ռ��˿�ͳ����Ϣ */
		portStatsCollector = this.threadPoolService
		.getScheduledExecutor()
		.scheduleAtFixedRate(new PortStatsCollector(), portStatsInterval, portStatsInterval, TimeUnit.SECONDS);

		logger.warn("Port statistics collection thread(s) started");
	}
	
	
	/**
	 * ��ȡ���н�������ͳ����Ϣ��ͨ������GetStatsThread�߳������stats_request�ķ��ͺ�stats_reply�Ľ��գ�
	 * ������I/O�������߳������
	 * @param allSwitchDpids
	 * @param statsType
	 * @return
	 */
	public Map<DatapathId, List<OFStatsReply>> getSwitchStatistics  
		(Set<DatapathId> dpids, OFStatsType statsType) {
		HashMap<DatapathId, List<OFStatsReply>> dpidRepliesMap = new HashMap<DatapathId, List<OFStatsReply>>();
		
		List<GetStatsThread> activeThreads = new ArrayList<GetStatsThread>(dpids.size());
		List<GetStatsThread> pendingRemovalThreads = new ArrayList<GetStatsThread>();
		GetStatsThread t;
		for (DatapathId d : dpids) {
			t = new GetStatsThread(d, statsType);
			activeThreads.add(t);
			t.start();
		}

		/* Join all the threads after the timeout. Set a hard timeout
		 * of 12 seconds for the threads to finish. If the thread has not
		 * finished the switch has not replied yet and therefore we won't
		 * add the switch's stats to the reply.
		 */
		for (int iSleepCycles = 0; iSleepCycles < portStatsInterval; iSleepCycles++) {
			/* ����activeThread������߳��Ƿ���ɣ���������¼dpid��replies������curThread���뵽pendingRemovalThreads�Դ���� */
			for (GetStatsThread curThread : activeThreads) {
				if (curThread.getState() == State.TERMINATED) {
					dpidRepliesMap.put(curThread.getSwitchId(), curThread.getStatsReplies());
					pendingRemovalThreads.add(curThread);
				}
			}

			/* remove the threads that have completed the queries to the switches */
			for (GetStatsThread curThread : pendingRemovalThreads) {
				activeThreads.remove(curThread);
			}
			
			/* clear the list so we don't try to double remove them */
			pendingRemovalThreads.clear();

			/* if we are done finish early */
			if (activeThreads.isEmpty()) {
				break;
			}

			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				logger.error("Interrupted while waiting for statistics", e);
			}
		}

		return dpidRepliesMap;
	}
	
	/**
	 * ��ȡһ̨��������ͳ����Ϣ
	 * ����stats_request������stats_reply
	 * @param dpid
	 * @param statsType
	 * @return
	 */
	@SuppressWarnings("unchecked")
	public List<OFStatsReply> getSwitchStatistics  
		(DatapathId dpid, OFStatsType statsType) {
		OFFactory my13Factory = OFFactories.getFactory(OFVersion.OF_13);
		OFStatsRequest<?> request = null;
		
		/* ����statsType���stats_request��Ϣ�ķ�װ */
		switch(statsType){
		case PORT:
			request = my13Factory.buildPortStatsRequest()
								 .setPortNo(OFPort.ANY)
								 .build();
			break;
			
		case FLOW:
			Match match = my13Factory.buildMatch().build();
			request = my13Factory.buildFlowStatsRequest()
								 .setMatch(match)
								 .setOutPort(OFPort.ANY)
								 .setOutGroup(OFGroup.ANY)
								 .setTableId(TableId.ALL)
								 .build();	
			break;
			
		default:
			logger.error("OFStatsType unknown,unable to build stats request");
		}
		
		/* ��ָ������������stats_request��������stats_reply */
		IOFSwitch sw = switchService.getSwitch(dpid);
		ListenableFuture<?> future = null;
		
		if(sw != null && sw.isConnected() == true)
			future = sw.writeStatsRequest(request);
		List<OFStatsReply> repliesList = null;
		try {
			repliesList = (List<OFStatsReply>) future.get(portStatsInterval*1000 / 2, TimeUnit.MILLISECONDS);
		} catch (Exception e) {
			logger.error("Failure retrieving statistics from switch {}. {}", sw, e);
		}
		return repliesList;
	}
	
	/**
	 * ��ȡͳ����Ϣ���߳���
	 * ��stats_msg��I/O���߳������
	 * ��run�����е���getSwitchStatistics�������stats_request�ķ��ͺ�stats_reply�Ľ���
	 *
	 */
	private class GetStatsThread extends Thread{
		private DatapathId dpid;
		private OFStatsType statsType;
		private	List<OFStatsReply> replies;
		
		public GetStatsThread(DatapathId dpid, OFStatsType statsType){
			this.dpid = dpid;
			this.statsType = statsType;
			this.replies = null;
		}
		
		public void run(){
			replies = getSwitchStatistics(dpid, statsType);
		}
		
		public DatapathId getSwitchId(){
			return dpid;
		}
		public List<OFStatsReply> getStatsReplies(){
			return replies;
		}
	}
}
