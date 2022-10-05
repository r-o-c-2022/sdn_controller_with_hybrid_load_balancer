package net.floodlightcontroller.trafficmonitor;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.projectfloodlight.openflow.protocol.OFFactories;
import org.projectfloodlight.openflow.protocol.OFFactory;
import org.projectfloodlight.openflow.protocol.OFMeterFlags;
import org.projectfloodlight.openflow.protocol.OFMeterMod;
import org.projectfloodlight.openflow.protocol.OFMeterModCommand;
import org.projectfloodlight.openflow.protocol.OFVersion;
import org.projectfloodlight.openflow.protocol.meterband.OFMeterBand;
import org.projectfloodlight.openflow.protocol.meterband.OFMeterBandDrop;
import org.projectfloodlight.openflow.types.DatapathId;
import org.projectfloodlight.openflow.types.OFPort;
import org.projectfloodlight.openflow.types.U64;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;

import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.core.internal.IOFSwitchService;
import net.floodlightcontroller.core.types.NodePortTuple;
import net.floodlightcontroller.linkdiscovery.ILinkDiscoveryService;
import net.floodlightcontroller.linkdiscovery.Link;

public class TrafficController {
	private static final Logger logger = LoggerFactory.getLogger(TrafficController.class);
	private static final String URL_ADD_DELETE_FLOW = "http://localhost:8080/wm/staticentrypusher/json";
	private static final String URL_TOPO_LINKS = "http://localhost:8080/wm/topology/links/json";
	private static int countFlow = 0;
	
	public static void executePolicy(IOFSwitchService switchService, ILinkDiscoveryService linkDiscoveryService, HashMap<NodePortTuple, SwitchPortStatistics> abnormalTraffic, HashMap<NodePortTuple, Date> addFlowEntryHistory, Policy policy, LinkedList<Event> events){		
		for(Entry<NodePortTuple, SwitchPortStatistics> e : abnormalTraffic.entrySet()){
			NodePortTuple npt = e.getKey();
			SwitchPortStatistics sps = e.getValue();
			IOFSwitch sw = switchService.getSwitch(npt.getNodeId());

			logger.info("ready to enter if");
			if(addFlowEntryHistory.isEmpty() || !addFlowEntryHistory.containsKey(npt)){	/* �·���ʷΪ�� ���� û���·����������� */

				/* �������ò���ִ��ϵͳ���� */
				switch(policy.getAction()){
				case Policy.ACTION_DROP:
					/* ��������쳣�Ķ˿������Զ�Ϊ�ն����������·�����  */
					if(!isPortConnectedToSwitch(sw.getId(), npt.getPortId(), linkDiscoveryService)){
						int hardTimeout = (int) policy.getActionDuration();
						dropPacket(sw, npt.getPortId().getPortNumber(), hardTimeout, countFlow++);					
						events.add(new Event(sps, policy));		/* ����쳣���������¼� */
						addFlowEntryHistory.put(npt, new Date());
					}
					break;
					
				case Policy.ACTION_LIMIT:
					logger.info("enter case ACTION_LIMIT");
					/* ��������쳣�Ķ˿������Զ�Ϊ�ն����������·�����  */
					if(!isPortConnectedToSwitch(sw.getId(), npt.getPortId(), linkDiscoveryService)){
						/* �·�meter���·�֮ǰ���meter���Ƿ���ڸ�meter��������£�������ӣ�OpenvSwitchĿǰ�ݲ�֧��meter�������ʸù����޷�ʵ�֣� */
						long meterId = 0, burstSize = 0;		
						meterId = addMeter(sw, policy.getRateLimit(), burstSize);
						logger.info("add Meter done");
						// �·��������meterId
						int hardTimeout1 = (int) policy.getActionDuration();
						rateLimit(sw, npt.getPortId().getPortNumber(), hardTimeout1, meterId, countFlow++);
						logger.info("limit Packet done");
						events.add(new Event(sps, policy));
						addFlowEntryHistory.put(npt, new Date());
					}
					break;
				
				default:
					logger.warn("undefined system action!");
					break;
				}				
			}else{	 /* �·���ʷ��Ϊ�����Ѿ���ö˿��·�������������������Ƿ���� */		
				Date currentTime = new Date();
				long period = (currentTime.getTime() - addFlowEntryHistory.get(npt).getTime()) / 1000;	// �����second
				if(policy.getActionDuration() > 0  && period > policy.getActionDuration()){
					logger.info("flow {match:" + npt.getNodeId() + " / " + npt.getPortId() + ", action:drop} expired!");
					addFlowEntryHistory.remove(npt);
				}//if
			}//else
		}//for
	}
	
	/**
	 * ���һ���������ƥ����˿ڵ����ݰ����ж���
	 * @param sw
	 * @param ingressPortNo
	 * @param hardTimeout
	 * @param countFlow
	 */
	public static void dropPacket(IOFSwitch sw, int ingressPortNo, int hardTimeout, int countFlow){
		//����������������
		HashMap<String, String> flow = new HashMap<String, String>();
		flow.put("switch", sw.getId().toString());
		flow.put("name", "flow" + countFlow);
		flow.put("in_port", String.valueOf(ingressPortNo));
		flow.put("cookie", "0");
		flow.put("priority", "32768");
		flow.put("active", "true");
		flow.put("hard_timeout", String.valueOf(hardTimeout));
		String result = addFlow(sw.getId().toString(), flow);
		logger.info(result);
	}
	
	/**
	 * ���һ���������meter���������󶨣��Ӷ�ʵ������
	 * @param sw
	 * @param ingressPortNo
	 * @param hardTimeout
	 * @param meterId
	 * @param countFlow
	 */
	public static void rateLimit(IOFSwitch sw, int ingressPortNo, int hardTimeout, long meterId, int countFlow){
		//����������������
		HashMap<String, String> flow = new HashMap<String, String>();
		flow.put("switch", sw.getId().toString());
		flow.put("name", "flow" + countFlow);
		flow.put("in_port", String.valueOf(ingressPortNo));
		flow.put("cookie", "0");
		flow.put("priority", "32768");
		flow.put("active", "true");
		flow.put("hard_timeout", String.valueOf(hardTimeout));
		flow.put("instruction_goto_meter", String.valueOf(meterId));
		String result = addFlow(sw.getId().toString(), flow);
		logger.info(result);
	}
	
	public static String addFlow(String did,HashMap<String,String> flow)
	{
		String result = sendPost(URL_ADD_DELETE_FLOW,hashMapToJson(flow));
		return result;
	}
	
	public static String sendPost(String url, String param) {
        PrintWriter out = null;
        BufferedReader in = null;
        String result = "";
        try {
            URL realUrl = new URL(url);
            // �򿪺�URL֮�������
            URLConnection conn = realUrl.openConnection();
            // ����POST�������������������
            conn.setDoOutput(true);
            conn.setDoInput(true);
            // ��ȡURLConnection�����Ӧ�������
            out = new PrintWriter(conn.getOutputStream());
            // �����������
            out.print(param);
            // flush������Ļ���
            out.flush();
            // ����BufferedReader����������ȡURL����Ӧ
            in = new BufferedReader(
                    new InputStreamReader(conn.getInputStream()));
            String line;
            while ((line = in.readLine()) != null) {
                result += line;
            }
        } catch (Exception e) {
            System.out.println("���� POST ��������쳣��"+e);
            e.printStackTrace();
        }
        //ʹ��finally�����ر��������������
        finally{
            try{
                if(out!=null){
                    out.close();
                }
                if(in!=null){
                    in.close();
                }
            }
            catch(IOException ex){
                ex.printStackTrace();
            }
        }
        return result;
    }
	
	public static String hashMapToJson(HashMap map) {  
        String string = "{";  
        for (Iterator it = map.entrySet().iterator(); it.hasNext();) {  
            Entry e = (Entry) it.next();  
            string += "\"" + e.getKey() + "\":";  
            string += "\"" + e.getValue() + "\",";  
        }  
        string = string.substring(0, string.lastIndexOf(","));  
        string += "}";  
        logger.info(string);
        return string;
    } 
	
	
	/**
	 * ���һ��meter���Գ���rate�����ݰ���ִ�ж���
	 * @param sw
	 * @param rateLimit
	 * @param burstSize
	 * @return	meterId
	 */
	public static long addMeter(IOFSwitch sw, U64 rateLimit, long burstSize){
		logger.info("enter add meter()");
		OFFactory my13Factory = OFFactories.getFactory(OFVersion.OF_13);
		/* ����flag */
		Set<OFMeterFlags> flags = new HashSet<OFMeterFlags>();
		flags.add(OFMeterFlags.KBPS);
//		flags.add(OFMeterFlags.BURST);

		long meterId = 1;
		
		/* rateLimit��Bps��ת����rate��kbps��*/
		long rate = (rateLimit.getValue() * 8) / 1000;
		
		/* ����һ��band */
		OFMeterBandDrop bandDrop = my13Factory.meterBands().buildDrop()
														   .setRate(rate)	// kbps	
//														   .setBurstSize(0)
														   .build();
		logger.info("create band");
		
		/* ����bands */
		List<OFMeterBand> bands = new ArrayList<OFMeterBand>();
		bands.add(bandDrop);		
		logger.info("add band to bands");
		
		/* ����һ��Meter Modification Message���������� */
		OFMeterMod meterMod = my13Factory.buildMeterMod()
										 .setMeterId(meterId)
										 .setCommand(OFMeterModCommand.ADD)
										 .setFlags(flags)
										 .setMeters(bands)
										 .build();
		logger.info("create meterMod msg");
	
		if( sw.write(meterMod) ){
			logger.info("add meter" + meterId + "to meter table");		
			return meterId;
		}
		
		else{
			logger.info("add meter failed");
			return 0;
		}
	}

	/**
	 * ��齻����sw�˿��������Ƿ�Ϊ������
	 * @return
	 */
	private static boolean isPortConnectedToSwitch(DatapathId dpid, OFPort port, ILinkDiscoveryService linkDiscoveryService){
		boolean result = false;
		
		Map linksMap = linkDiscoveryService.getLinks();	/* ��ȡ������֮�����· */
		Set keys = linksMap.keySet();
		Iterator it = keys.iterator();
		while(it.hasNext()){
			Link link = (Link)it.next();
			/* ���ý�������·���Ƿ���dpid��portNumber*/
			result = (link.getSrc().equals(dpid) && link.getSrcPort().equals(port)) || (link.getDst().equals(dpid) && link.getDstPort().equals(port) )? true : false;
		}
		return result;
	}
}


