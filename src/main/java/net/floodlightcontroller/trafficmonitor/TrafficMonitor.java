package net.floodlightcontroller.trafficmonitor;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

import org.projectfloodlight.openflow.protocol.OFMessage;
import org.projectfloodlight.openflow.protocol.OFType;
import org.projectfloodlight.openflow.types.DatapathId;
import org.projectfloodlight.openflow.types.OFPort;
import org.projectfloodlight.openflow.types.U64;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;



import net.floodlightcontroller.core.FloodlightContext;
import net.floodlightcontroller.core.IOFMessageListener;
import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.core.internal.IOFSwitchService;
import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import net.floodlightcontroller.core.types.NodePortTuple;
import net.floodlightcontroller.linkdiscovery.ILinkDiscoveryService;
import net.floodlightcontroller.restserver.IRestApiService;
import net.floodlightcontroller.threadpool.IThreadPoolService;
import net.floodlightcontroller.trafficmonitor.web.TrafficMonitorWebRoutable;



public class TrafficMonitor implements IOFMessageListener, IFloodlightModule, ITrafficMonitorService {
	
	protected static final Logger logger = LoggerFactory.getLogger(TrafficMonitor.class);
	
	private IOFSwitchService 			switchService;
	private IThreadPoolService 			threadPoolService;		// �̳߳�
	private IRestApiService				restApiService;
	private ILinkDiscoveryService 		linkDiscoveryService;
	
	private static Policy 				policy = new Policy();			   // Ĭ��ִ�ж�����ʱ��30s
	private static LinkedList<Event>	events = new LinkedList<Event>();  // �洢�������¼�����¼ϵͳ�����쳣������ʱ�估������
	private TrafficCollector			trafficCollector = new TrafficCollector();

	/*
	 * IFloodlightModule��ʵ��
	 */
	
	/**
	 * ��OFMessage��������һ��ID
	 */
	@Override
	public String getName() {
		// TODO Auto-generated method stub
		return TrafficMonitor.class.getSimpleName();
	}

	@Override
	public boolean isCallbackOrderingPrereq(OFType type, String name) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public boolean isCallbackOrderingPostreq(OFType type, String name) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleServices() {
		// TODO Auto-generated method stub
		Collection<Class<? extends IFloodlightService>> l = new
				ArrayList<Class<? extends IFloodlightService>>();
				l.add(ITrafficMonitorService.class);
				return l;
	}
	
	/**
	 * tells the module system that we are the class that provides the service
	 * ����ģ��ϵͳTrafficMonitor���ṩITrafficMonitorService����
	 */
	@Override
	public Map<Class<? extends IFloodlightService>, IFloodlightService> getServiceImpls() {
		// TODO Auto-generated method stub
		Map<Class<? extends IFloodlightService>, IFloodlightService> m = new
				HashMap<Class<? extends IFloodlightService>, IFloodlightService>();
				m.put(ITrafficMonitorService.class, this);
				return m;
	}

	
	/**
	 * ��֪ģ���������floodlight����ʱ����ģ����ء�
	 */
	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleDependencies() {
		// TODO Auto-generated method stub
		   Collection<Class<? extends IFloodlightService>> l =
			        new ArrayList<Class<? extends IFloodlightService>>();
			    l.add(IOFSwitchService.class);
			    l.add(IThreadPoolService.class);
			    l.add(IRestApiService.class);
			    l.add(ILinkDiscoveryService.class);
			    return l;
	}

	/**
	 * ���ڿ������������ڱ����ã�����Ҫ�����Ǽ���������ϵ����ʼ�����ݽṹ��
	 */
	@Override
	public void init(FloodlightModuleContext context)
			throws FloodlightModuleException {
		// TODO Auto-generated method stub
		switchService = context.getServiceImpl(IOFSwitchService.class);
		threadPoolService = context.getServiceImpl(IThreadPoolService.class);
		restApiService = context.getServiceImpl(IRestApiService.class);	
		linkDiscoveryService = context.getServiceImpl(ILinkDiscoveryService.class);
	}


	/**
	 * ģ������
	 */
	@Override
	public void startUp(FloodlightModuleContext context){
		/* ��������url */
		restApiService.addRestletRoutable(new TrafficMonitorWebRoutable());
		
		/* �ռ��������˿�ͳ����Ϣ������port_speed */
		trafficCollector.startPortStatsCollection(switchService, threadPoolService, linkDiscoveryService, policy, events);
		
	}
	
	/**
	 * �� OF��Ϣ�Ĵ�����Ҫ��PACKET_IN��Ϣ��
	 */
	@Override
	public net.floodlightcontroller.core.IListener.Command receive(
			IOFSwitch sw, OFMessage msg, FloodlightContext cntx) {
		// TODO Auto-generated method stub
		return null;
	}
	
	
	/*
	 * ITrafficMonitorService ʵ��
	 */
	@Override
	public HashMap<NodePortTuple, SwitchPortStatistics> getPortStatistics() {
		// TODO Auto-generated method stub
		return TrafficCollector.prePortStatsBuffer;
	}	
	
	@Override
	public SwitchPortStatistics getPortStatistics(DatapathId dpid, OFPort port) {
		// TODO Auto-generated method stub
		return TrafficCollector.prePortStatsBuffer.get(new NodePortTuple(dpid, port));
	}


	@Override
	public Policy getPolicy() {
		// TODO Auto-generated method stub
		return policy;
	}

	@Override
	public void setPolicy(U64 trafficThreshold, String action,
			long actionDuration, U64 rateLimit) {
		policy.setTrafficThreshold(trafficThreshold);
		policy.setAction(action);
		policy.setActionDuration(actionDuration);
		policy.setRateLimit(rateLimit);
		
	}

	@Override
	public LinkedList<Event> getEvents() {
		// TODO Auto-generated method stub
		return events;
	}

}

