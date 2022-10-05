package net.floodlightcontroller.trafficmonitor;

import java.util.HashMap;
import java.util.Map.Entry;

import net.floodlightcontroller.core.types.NodePortTuple;

import org.projectfloodlight.openflow.types.U64;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


public class TrafficAnalyzer {
	private static final Logger logger = LoggerFactory.getLogger(TrafficAnalyzer.class);

	public static void Analysis(HashMap<NodePortTuple, SwitchPortStatistics> portStats, HashMap<NodePortTuple, SwitchPortStatistics> abnormalTraffic, Policy policy){
		U64 rxSpeed = U64.ZERO, txSpeed = U64.ZERO;
		U64 trafficThreshold = policy.getTrafficThreshold();
		String print = "\n";		
		/* ���ݶ˿���������ֵ�Ƚ����ж��Ƿ�Ϊ�쳣���� */
		for(Entry<NodePortTuple, SwitchPortStatistics> e : portStats.entrySet()){
			rxSpeed = e.getValue().getRxSpeed();	/* �뷽������  */
			txSpeed = e.getValue().getRxSpeed();	/* ����������  */
			print += e.getKey().getNodeId() +" / " + e.getKey().getPortId() + " rxSpeed: " + rxSpeed.getBigInteger().toString() + " txSpeed: " + txSpeed.getBigInteger().toString() +"\n";
			if(rxSpeed.compareTo(trafficThreshold) > 0 || txSpeed.compareTo(trafficThreshold) > 0){
				abnormalTraffic.put(e.getKey(), e.getValue());
			}
		}
		logger.info(print);
	}
}
