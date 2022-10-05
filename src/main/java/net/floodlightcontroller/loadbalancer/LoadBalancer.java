/**
 *    Copyright 2013, Big Switch Networks, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may
 *    not use this file except in compliance with the License. You may obtain
 *    a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *    License for the specific language governing permissions and limitations
 *    under the License.
 **/

package net.floodlightcontroller.loadbalancer;


import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

import org.projectfloodlight.openflow.protocol.OFFlowMod;
import org.projectfloodlight.openflow.protocol.match.Match;
import org.projectfloodlight.openflow.protocol.match.MatchField;
import org.projectfloodlight.openflow.protocol.OFBucket;
import org.projectfloodlight.openflow.protocol.OFFactory;
import org.projectfloodlight.openflow.protocol.OFGroupProp;
import org.projectfloodlight.openflow.protocol.OFGroupType;
import org.projectfloodlight.openflow.protocol.OFMessage;
import org.projectfloodlight.openflow.protocol.OFPacketIn;
import org.projectfloodlight.openflow.protocol.OFPacketOut;
import org.projectfloodlight.openflow.protocol.OFType;
import org.projectfloodlight.openflow.protocol.OFVersion;
import org.projectfloodlight.openflow.protocol.action.OFAction;
import org.projectfloodlight.openflow.types.DatapathId;
import org.projectfloodlight.openflow.types.EthType;
import org.projectfloodlight.openflow.types.IPv4Address;
import org.projectfloodlight.openflow.types.IpProtocol;
import org.projectfloodlight.openflow.types.MacAddress;
import org.projectfloodlight.openflow.types.OFBufferId;
import org.projectfloodlight.openflow.types.OFGroup;
import org.projectfloodlight.openflow.types.OFPort;
import org.projectfloodlight.openflow.types.TableId;
import org.projectfloodlight.openflow.types.TransportPort;
import org.projectfloodlight.openflow.types.U16;
import org.projectfloodlight.openflow.types.U64;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;

import net.floodlightcontroller.core.FloodlightContext;
import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.IOFMessageListener;
import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.core.internal.IOFSwitchService;
import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import net.floodlightcontroller.core.types.NodePortTuple;
import net.floodlightcontroller.debugcounter.IDebugCounter;
import net.floodlightcontroller.debugcounter.IDebugCounterService;
import net.floodlightcontroller.debugcounter.IDebugCounterService.MetaData;
import net.floodlightcontroller.devicemanager.IDevice;
import net.floodlightcontroller.devicemanager.IDeviceService;
import net.floodlightcontroller.devicemanager.SwitchPort;
import net.floodlightcontroller.packet.ARP;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.ICMP;
import net.floodlightcontroller.packet.IPacket;
import net.floodlightcontroller.packet.IPv4;
import net.floodlightcontroller.packet.TCP;
import net.floodlightcontroller.packet.UDP;
import net.floodlightcontroller.restserver.IRestApiService;
import net.floodlightcontroller.routing.IRoutingService;
import net.floodlightcontroller.routing.Path;
import net.floodlightcontroller.staticentry.IStaticEntryPusherService;
import net.floodlightcontroller.statistics.IStatisticsService;
import net.floodlightcontroller.statistics.SwitchPortBandwidth;
import net.floodlightcontroller.topology.ITopologyService;
import net.floodlightcontroller.util.FlowModUtils;
import net.floodlightcontroller.util.OFMessageUtils;

/**
 * A simple load balancer module for ping, tcp, and udp flows. This module is accessed 
 * via a REST API defined close to the OpenStack Quantum LBaaS (Load-balancer-as-a-Service)
 * v1.0 API proposal. Since the proposal has not been final, no efforts have yet been 
 * made to confirm compatibility at this time. 
 * 
 * Limitations:
 * - client records and static flows not purged after use, will exhaust switch flow tables over time
 * - round robin policy among servers based on connections, not traffic volume
 * - health monitoring feature not implemented yet
 *  
 * @author kcwang
 * @edited Ryan Izard, rizard@g.clemson.edu, ryan.izard@bigswitch.com
 */
public class LoadBalancer implements IFloodlightModule,
ILoadBalancerService, IOFMessageListener {

	protected static Logger log = LoggerFactory.getLogger(LoadBalancer.class);

	// Our dependencies
	protected IFloodlightProviderService floodlightProviderService;
	protected IRestApiService restApiService;

	protected IDebugCounterService debugCounterService;
	private IDebugCounter counterPacketOut;
	protected IDeviceService deviceManagerService;
	protected IRoutingService routingEngineService;
	protected ITopologyService topologyService;
	protected IStaticEntryPusherService sfpService;
	protected IOFSwitchService switchService;
	protected IStatisticsService statisticsService;

	protected HashMap<String, LBVip> vips;
	protected HashMap<String, LBPool> pools;
	protected HashMap<String, LBMember> members;
	protected HashMap<Integer, String> vipIpToId;
	protected HashMap<Integer, MacAddress> vipIpToMac;
	protected HashMap<Integer, String> memberIpToId;
	protected HashMap<IPClient, LBMember> clientToMember;

	//Copied from Forwarding with message damper routine for pushing proxy Arp 
	protected static String LB_ETHER_TYPE = "0x800";
	protected static int LB_PRIORITY = 32768;

	
	// Comparator for sorting by SwitchCluster
	public Comparator<SwitchPort> clusterIdComparator =
			new Comparator<SwitchPort>() {
		@Override
		public int compare(SwitchPort d1, SwitchPort d2) {
			DatapathId d1ClusterId = topologyService.getClusterId(d1.getNodeId());
			DatapathId d2ClusterId = topologyService.getClusterId(d2.getNodeId());
			return d1ClusterId.compareTo(d2ClusterId);
		}
	};

	// data structure for storing connected
	public class IPClient {
		IPv4Address ipAddress;
		IpProtocol nw_proto;
		TransportPort srcPort; // tcp/udp src port. icmp type (OFMatch convention)
		TransportPort targetPort; // tcp/udp dst port, icmp code (OFMatch convention)

		public IPClient() {
			ipAddress = IPv4Address.NONE;
			nw_proto = IpProtocol.NONE;
			srcPort = TransportPort.NONE;
			targetPort = TransportPort.NONE;
		}
	}


	// ROC
	
 	int[] nArrServerPort = {5, 6, 7, 8, 9, 10, 11, 12};
 	public static final int CLIENT_PORT = 4;
 	public int SERVER_CNT = 8;
 	// public static final String SWITCH_ID = "00:00:ce:41:b7:31:2d:4c";
 	public static final String SWITCH_ID = "00:00:00:0c:29:ea:48:9c";
 	final double IMBALANCE_THRESHOLD_CEILING_VAL = 0.01, IMBALANCE_THRESHOLD_FLOOR_VAL = 0.01; // Origin: (10, 5)
 	String[] arrHostIPAddr = {"192.168.198.151", "192.168.198.152", "192.168.198.153", "192.168.198.154", 
 			"192.168.198.155", "192.168.198.156", "192.168.198.157", "192.168.198.158"};
 	String[] arrHostMACAddr = {"08:00:27:79:7e:94", "08:00:27:b4:48:40", "08:00:27:4a:cd:58", "08:00:27:2c:bc:ba", 
 			"08:00:27:5b:60:62", "08:00:27:c1:d3:6a", "08:00:27:cd:2a:05", "08:00:27:97:88:5e"};
 	
 	final int SERVERHOST_SERVICE_PORT = 53536;
 	int[] nArrFailoverGroupTableID = {101, 102, 103, 104, 105, 106, 107, 108};
 	public static final int EXP_SERVERLOAD = 100000;
 	public static final double EXP_OFFSET = 0.00001;
 	public static final double PRECISION_SERVERLOAD = 0.03; // 0.00001;
 	final String VIP_ADDRESS = "192.168.198.160";
 	final String CLIENT_ADDRESS = "192.168.198.120";
 	
 	class ServerMonitoring extends Thread {
 		LBPool lbpool;
 		DatapathId switchDPId;
 		IOFSwitch ofSwitch = null;
 		OFFactory myFactory = null;
 		byte bIsStatic = 1, bIsFlowTableInited = 0;
 		final int FAILOVER_GROUPTABLE_ENTITY_CNT = 2, SELECT_GROUPTABLE_ID = 100;
 		final int MONITOR_INTERVAL = 1000 * 10;
 		ServerMonitoring(LBPool tmpLBPool, String string) {
 			if (null == tmpLBPool && null == string) return;
 			
 			lbpool = tmpLBPool;
 			switchDPId = DatapathId.of(string);
 		}
 		
 		/**
 		 * http://localhost:9090/api/v1/query?query=100*avg+by+%28instance%29+%28rate%28node_cpu_seconds_total%7Bmode%21%3D%22idle%22%7D%5B1m%5D%29%29
 		 */
 		@Override
 		public void run() {
 			System.out.println("Server Monitoring Thread is Starting...");
 			
 			try (BufferedWriter writer = new BufferedWriter(new FileWriter("/home/roc/imbalance_log.txt", true))) {
 				writer.append(  "=========================\n" + new Date().toString() + ": Start logging...\n");
 			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
 			
 			int nElapsedTmMs = 0;
 			double dblLoadImbSum = 0;
 	 		int nMonitorCnt = 0;
 			
 			ServerMonitoring test = new ServerMonitoring(null, null);
 			while (true) {
 				nElapsedTmMs += MONITOR_INTERVAL;
 				
 				// If ofSwitch is not set yet, then don't go on. Try again after MONITOR_INTERVAL.
 				if (null == ofSwitch) {
 					ofSwitch = switchService.getSwitch(switchDPId);
 					if (null == ofSwitch) {
 						try {
 		 					Thread.sleep(MONITOR_INTERVAL);
 		 				} catch (InterruptedException e) {
 		 					// TODO Auto-generated catch block
 		 					e.printStackTrace();
 		 				}
 						
 						log.info("ServerMonitoring:run() => ofSwitch is null..");
 						
 						continue;
 					}
 					myFactory = ofSwitch.getOFFactory();
 					
 					log.info("ServiceMonitor:run() => OpenFlow Version: {}", myFactory.getVersion());
 				}
 				
 				
 				
 				
 		        String strResponse = test.doGet("http://localhost:9090/api/v1/query?query=100*avg+by+%28instance%29+%28rate%28node_cpu_seconds_total%7Bmode%21%3D%22idle%22%7D%5B1m%5D%29%29");
 		        
 		        // System.out.println( "ServiceMonitor:run() => Prometheus respons: " + strResponse);
 		        
 		        JSONObject jsonObject = JSONObject.parseObject(strResponse);
 		        JSONArray arrJsonResult = ( (JSONObject) jsonObject.get("data")).getJSONArray("result");
 		        
 		        int i = 0;
 		        int nServerCnt = 0;
 		        byte bIsLoadStatusChanged = 0; // save the flag if the load status of server cluster has been changed
 		        
 		        for (Object objItr : arrJsonResult) {
 		        	JSONObject objJsonResult = (JSONObject)objItr;
 		        	String strInst = ((JSONObject)objJsonResult.get("metric")).get("instance").toString();
 		        	
 		        	// Get the server id from JSON response using server's ip address. 
 		        	int nCurServerId = 0;
 		        	while (nCurServerId < SERVER_CNT) {
 		        		if (strInst.contains(arrHostIPAddr[nCurServerId])) break;
 		        		nCurServerId ++;
 		        	}
 		        	
 		        	// If SERVERCNT==nCurServerID, it means can not find ther server's ip address in the arrHostIpAddr[].
 		        	if (SERVER_CNT == nCurServerId) {
 		        		log.warn("ServiceMonitor:run() => Can't find the " + strInst + " in the arrHostIPAddr[]");
 		        		break;
 		        	}
 		        	
 		        	double dblCurLoad = Float.valueOf( objJsonResult.getJSONArray("value").get(1).toString() ) / 100.0;
 		        	
 		        	// if the current load is not same as the previous one.
 		        	if ( Math.abs( lbpool.dblArrDynWeight[nCurServerId] - dblCurLoad ) > PRECISION_SERVERLOAD) {
 		        		bIsLoadStatusChanged = 1;
 		        	}
 		        	
 		        	while (nServerCnt < nCurServerId) {
 		        		lbpool.dblArrDynWeight[nServerCnt] = 1.0;
 		        		if (0 == nServerCnt)
 		        			lbpool.dblArrAccuDynWeight[nServerCnt] = 0;
 		        		else lbpool.dblArrAccuDynWeight[nServerCnt] = lbpool.dblArrAccuDynWeight[nServerCnt - 1];
 		        		nServerCnt ++;
 		        	}
 		        	nServerCnt ++;
 		        	
 		        	lbpool.dblArrDynWeight[nCurServerId] = dblCurLoad;
 		        	if (0 == nCurServerId) lbpool.dblArrAccuDynWeight[nCurServerId] = 1.0 - dblCurLoad;
 		        	else lbpool.dblArrAccuDynWeight[nCurServerId] = 1.0 - dblCurLoad + lbpool.dblArrAccuDynWeight[nCurServerId - 1];
 		        	
 		        	System.out.println( "ServiceMonitor:run() => Load of the server " + strInst + 
 		        						"(" + String.valueOf(nCurServerId)  + "th server):" + String.valueOf(dblCurLoad) + "%" );
 		        }
 		        
 		        if (0 == nServerCnt) {
 		        	log.info("ServiceMonitor:run() => No prometheus response." );
 		        	
 		        	try {
		 					Thread.sleep(MONITOR_INTERVAL);
		 				} catch (InterruptedException e) {
		 					// TODO Auto-generated catch block
		 					e.printStackTrace();
		 				}
 		        	continue;
 		        }
 		        
 		        // Fill the rest server load status list.
 		       while (nServerCnt < SERVER_CNT) {
	        		lbpool.dblArrDynWeight[nServerCnt] = 1.0;
	        		lbpool.dblArrAccuDynWeight[nServerCnt] = lbpool.dblArrAccuDynWeight[nServerCnt - 1];
	        		nServerCnt ++;
	        	}
 		        
 		        // If the number of servers in prometheus response is not same with the SERVER_CNT, 
 		        // then don't go on. Try again after MONITOR_INTERVAL.
 		        if (nServerCnt != SERVER_CNT) {
 		        	log.warn( "ServiceMonitor:run() => The server count in prometheus ({}) is not same as SERVER_CNT. ", i);
 		        	
 		        	try {
		 					Thread.sleep(MONITOR_INTERVAL);
		 				} catch (InterruptedException e) {
		 					// TODO Auto-generated catch block
		 					e.printStackTrace();
		 				}
 		        	
 		        	continue;
 		        }
 		        
 		        log.info( "ServiceMonitor:run() => dblArrAccuDynWeight[]" );
 		        for (i = 0; i < SERVER_CNT; i ++) {
 		        	log.info("dblArrAccyDynWeight[{}]={}", i, lbpool.dblArrAccuDynWeight[i]);
 		        }
 		        
 		        double dblLoadImb = GetLoadImbalanceVal(lbpool.dblArrDynWeight, SERVER_CNT);
		        
 		       try (BufferedWriter writer = new BufferedWriter(new FileWriter("/home/roc/imbalance_log.txt", true))) {
 	 				writer.append(  new Date().toString() + ": Current status(" + String.format("Elapsed time:%dms", nElapsedTmMs - MONITOR_INTERVAL) + ")\n");
 	 				
 	 				for (i = 0; i < SERVER_CNT; i ++) {
 	 					writer.append( String.format("dblArrAccyDynWeight[%d]=%f\n",  i, lbpool.dblArrAccuDynWeight[i]) );
 	 		        }
 	 				writer.append( String.format("Current imbalance value=%f\n",  dblLoadImb));
 	 				
 	 			} catch (IOException e1) {
 					// TODO Auto-generated catch block
 					e1.printStackTrace();
 				}
 		        
				dblLoadImbSum += dblLoadImb;
				nMonitorCnt ++;
				
				log.info("ServiceMonitor:run() => Current imbalance value:{}, Average imbalance value:{}", dblLoadImb, dblLoadImbSum / nMonitorCnt);
				
 		        if (lbpool.lbMethod != LBPool.DWRS_BINSEARCH) {
 		        	try {
 	 					Thread.sleep(MONITOR_INTERVAL);
 	 				} catch (InterruptedException e) {
 	 					// TODO Auto-generated catch block
 	 					e.printStackTrace();
 	 				}
 		        	
 		        	continue;
 		        }
 		        
 		        // if the load status of server cluster has been changed, then we have to reset the list of fail-over group tables
 		        
 		        if (1 == bIsLoadStatusChanged) {
 		        	
 		        	// First, delete the previously set ff group table.
 		        	for (i = 0; i < SERVER_CNT; i ++) {
 		        		ofSwitch.write(myFactory.buildGroupDelete().setGroup(OFGroup.of(nArrFailoverGroupTableID[i])).setGroupType(OFGroupType.FF).build() );
 		        	}
 		        	
 		        	// For every server, set the corresponding ff group table entry.
 		        	// We first sort the array of the server load status to reduce the overhead for finding the nearest server.
 		        	
 		        	for (i = 0; i < SERVER_CNT; i ++) {
 		        		
 		        		// If the current server's load is 0, then continue.
 		        		if ( lbpool.dblArrDynWeight[i] <= EXP_OFFSET ) {
 		        			continue;
 		        		}
 		        		
 		        		// Find the server whose load is the nearest to the ith server.
 		        		// The result is assigned to nCandiServerId.
 		        		int nCandiServerId = -1;
 		        		double dblDiffMxVal = Double.MAX_VALUE;
 		        		
 		        		for (int j = 0; j < SERVER_CNT; j ++) {
 		        			if (i != j 
 		        					&& lbpool.dblArrDynWeight[j] > EXP_OFFSET	// server's load status has not to be 0.
 		        					&& Math.abs(lbpool.dblArrDynWeight[i] - lbpool.dblArrDynWeight[j]) < dblDiffMxVal) {
 		        				dblDiffMxVal = Math.abs(lbpool.dblArrDynWeight[i] - lbpool.dblArrDynWeight[j]);
 		        				nCandiServerId = j;
 		        			}
 		        		}
 		        		
 		        		// Set the ff group table buckets
 		        		ArrayList<OFBucket> buckets = new ArrayList<OFBucket>();
 		        		
 		        		// Actions for the first bucket which output the packet to the ith server.
 		        		ArrayList<OFAction> actionForFFGroupTable1 = new ArrayList<OFAction>();
 		        		ArrayList<OFAction> actionForFFGroupTable2 = new ArrayList<OFAction>();
 		        		
 	 			    	if (myFactory.getVersion().compareTo(OFVersion.OF_12) < 0) {
 	 			    		actionForFFGroupTable1.add(myFactory.actions().setDlDst( MacAddress.of( arrHostMACAddr[i] ) ));
 	 			    		actionForFFGroupTable1.add(myFactory.actions().setNwDst( IPv4Address.of( arrHostIPAddr[i] ) ));
 	 					} else { // OXM introduced in OF1.2
 	 						actionForFFGroupTable1.add(myFactory.actions().setField(myFactory.oxms().ethDst(MacAddress.of( arrHostMACAddr[i] ))));
 	 						actionForFFGroupTable1.add(myFactory.actions().setField(myFactory.oxms().ipv4Dst(IPv4Address.of( arrHostIPAddr[i] ))));
 	 					}
 	 			    	actionForFFGroupTable1.add( ofSwitch.getOFFactory().actions().buildOutput()
 		    				.setPort ( OFPort.of( nArrServerPort[i] ) ).setMaxLen(Integer.MAX_VALUE).build() );
 		    			
 		        		// set actions with set_field and output
 		        		buckets.add(myFactory.buildBucket()
 		        				.setWatchPort( OFPort.of(nArrServerPort[i]) ).setWatchGroup(OFGroup.ZERO)
 				        	    .setActions(actionForFFGroupTable1)
 				        	    .build() );
 		        		
 		        		if (-1 != nCandiServerId) {
 		        			// Actions for the second bucket which output the packet to the nCandiServerId th server.
 		        			
 		        			if (myFactory.getVersion().compareTo(OFVersion.OF_12) < 0) {
 		        				actionForFFGroupTable2.add(myFactory.actions().setDlDst( MacAddress.of( arrHostMACAddr[ nCandiServerId ] ) ));
 		        				actionForFFGroupTable2.add(myFactory.actions().setNwDst( IPv4Address.of( arrHostIPAddr[ nCandiServerId ] ) ));
 	 	 					} else { // OXM introduced in OF1.2
 	 	 						actionForFFGroupTable2.add(myFactory.actions().setField(myFactory.oxms().ethDst(MacAddress.of( arrHostMACAddr[nCandiServerId] ))));
 	 	 						actionForFFGroupTable2.add(myFactory.actions().setField(myFactory.oxms().ipv4Dst(IPv4Address.of( arrHostIPAddr[nCandiServerId] ))));
 	 	 					}
 	 	 			    	actionForFFGroupTable2.add( ofSwitch.getOFFactory().actions().buildOutput()
 	 		    				.setPort ( OFPort.of( nArrServerPort[nCandiServerId] ) ).setMaxLen(Integer.MAX_VALUE).build() );
 		        			
 		        			buckets.add(myFactory.buildBucket()
 			        				.setWatchPort( OFPort.of(nArrServerPort[nCandiServerId]) ).setWatchGroup(OFGroup.ZERO)
 					        	    .setActions(actionForFFGroupTable2)
 					        	    .build() );
 		        		} else {
 		        			log.warn("ServiceMonitor:run() => Can not get nCandiServerId.");
 		        		}
 		        		
 		        		boolean res = ofSwitch.write(myFactory.buildGroupAdd()
 			        			.setGroup(OFGroup.of(nArrFailoverGroupTableID[i]))
 			        			.setGroupType(OFGroupType.FF)
 			        			.setBuckets(buckets)
 			        			.build());
 		        		
 		        		if (!res) {
 		        			log.warn("ServiceMonitor:run() => Can not bucket of candidate server in failover group table {}.", nArrFailoverGroupTableID[i]);
 		        		}
 		        		
 		        		actionForFFGroupTable1 = null;
 		        		actionForFFGroupTable2 = null;
 		        		
 		        		// break;
 		        	}
 		        	
 		        	// break;
 		        }
 		        
 		        //   Dynamic => Static
 		        if ( 0 == bIsFlowTableInited || (0 == bIsStatic && dblLoadImb < IMBALANCE_THRESHOLD_FLOOR_VAL) ) {
 		        	log.info("ServiceMonitor:run() => Converting to static LB now. IMBALANCE value:{}", dblLoadImb);
 		        	
 		        	if (0 == bIsFlowTableInited) {
 		        		bIsFlowTableInited = 1;
 		        	}
 		        	
 		        	bIsStatic = 1;
 		        	
 		        	// First, delete the flow entry which was set in the dynamic LB mode.
 		        	ofSwitch.write(myFactory.buildFlowDelete()
 		        			.setMatch(myFactory.buildMatch()
 		        					.setExact(MatchField.ETH_TYPE, EthType.IPv4)
 		        					.setExact(MatchField.IPV4_DST, IPv4Address.of( CLIENT_ADDRESS ) )
 		        					.build())
 		        			.build());
 		        	ofSwitch.write(myFactory.buildFlowDelete()
 		        			.setMatch(myFactory.buildMatch()
 		        					.setExact(MatchField.ETH_TYPE, EthType.IPv4)
 		        					.setExact(MatchField.IPV4_DST, IPv4Address.of( VIP_ADDRESS ) )
 		        					.build())
 		        			.build());
 		        	
 		        	// log.info( "ServiceMonitor:run() => Flow table entry set in dynamic LB algorithm has been removed." );
 		        	
 		        	// Second, insert the "SELECT" group table whose every bucket action is the Goto-Table action.
 		        	// Each Goto-Table action  forwards the packet to the fast failover group table corresponding 
 		        	// to the each server.  
 		        	
 		        	ArrayList<OFBucket> buckets = new ArrayList<OFBucket>(SERVER_CNT);
 		        	
 		        	for (i = 0; i < SERVER_CNT; i ++) {
 		        		
 		        		// Append Goto-Table action to the buckets.
 		        		buckets.add(myFactory.buildBucket()
 				        	    .setActions(
 				        	    		Collections.singletonList(
 				        	    				(OFAction) ofSwitch.getOFFactory().actions()
 				        	    				.buildGroup().setGroup( OFGroup.of(nArrFailoverGroupTableID[i]) ).build()
 				        	    				)
 				        	    		)
 				        	    .setWatchGroup(OFGroup.ANY)
 				        	    .setWatchPort(OFPort.ANY)
 				        	    .setWeight(1)
 				        	    .build() );
 		        	}
 		        	
 			    	boolean res = ofSwitch.write(myFactory.buildGroupAdd()
 		        			.setGroup(OFGroup.of(SELECT_GROUPTABLE_ID))
 		        			.setGroupType(OFGroupType.SELECT)
 		        			.setBuckets(buckets)
 		        			.build());
 			    	
 			    	if (!res) {
	        			log.warn("ServiceMonitor:run() => Can not add select group table for static LB.");
	        		}
 			    	
 			    	/*
 			    	log.info( "ServiceMonitor:run() => Select group table entry for {}th server has been written." );
	        		log.info("ServiceMonitor:run() => buckets: {}", buckets);
	        		log.info("ServiceMonitor:run() => Result: {}", res);
 			    	*/
	        		// Next, insert the flow entry which forwards the client request packets to the "select" group table.
	        		
 			    	res = ofSwitch.write(
 			    			myFactory.buildFlowAdd()
 		        		    .setMatch(myFactory.buildMatch()
 		        		    		.setExact(MatchField.ETH_TYPE, EthType.IPv4)
 		        		    		.setExact(MatchField.IP_PROTO, IpProtocol.TCP)
 		        		    		.setExact(MatchField.IPV4_DST, IPv4Address.of( VIP_ADDRESS ))
 		        		    		.setExact(MatchField.TCP_DST, TransportPort.of( SERVERHOST_SERVICE_PORT ) )
 		        		    		.build())
 		        		    .setPriority(FlowModUtils.PRIORITY_MAX)
 		        		    .setActions(
 		        		    		Collections.singletonList(
	        		    				(OFAction) ofSwitch.getOFFactory().actions().buildGroup().setGroup( 
	        		    							OFGroup.of(SELECT_GROUPTABLE_ID)
	        		    							// OFGroup.of(101)
	        		    						).build()
        		    				)
 		        		    	)
 		        		    .build()
 			    			);
 			    	
 			    	if (!res) {
	        			log.warn("ServiceMonitor:run() => Can not add flow entry for forwarding to SELECT Group Table for static LB.");
	        		}
 			    	
 			    	// log.info( "ServiceMonitor:run() => Flow entry action for going to select table is configured." );
	        		// log.info("ServiceMonitor:run() => Result: {}", res);
	        		
 					// Next, insert the flow entry with set_field action.
	        		// This action replaces the source IPv4 and source MAC of the packet with the IPv4 and MAC corresponded to the total server cluster.
 			    	ArrayList<OFAction> actions = new ArrayList<OFAction>();
 			    	
 			    	if (myFactory.getVersion().compareTo(OFVersion.OF_12) < 0) {
 						actions.add(myFactory.actions().setDlSrc(MacAddress.of(LBVip.LB_PROXY_MAC)));
 						actions.add(myFactory.actions().setNwSrc(IPv4Address.of(VIP_ADDRESS)));
 					} else { // OXM introduced in OF1.2
 						actions.add(myFactory.actions().setField(myFactory.oxms().ethSrc(MacAddress.of( LBVip.LB_PROXY_MAC ))));
 						actions.add(myFactory.actions().setField(myFactory.oxms().ipv4Src(IPv4Address.of( VIP_ADDRESS ))));
 					}
 			    	actions.add(myFactory.actions().output(OFPort.of(CLIENT_PORT), Integer.MAX_VALUE));
 			    	
 			    	res = ofSwitch.write(
 			    			myFactory.buildFlowAdd()
 		        		    .setMatch( myFactory.buildMatch()
 		        		    		.setExact(MatchField.ETH_TYPE, EthType.IPv4)
 		        		    		.setExact(MatchField.IP_PROTO, IpProtocol.TCP)
 		        		    		.setExact(MatchField.IPV4_DST, IPv4Address.of(CLIENT_ADDRESS))
 		        		    		.setExact(MatchField.TCP_SRC, TransportPort.of( SERVERHOST_SERVICE_PORT ) )
 		        		    		.build() )
 		        		    .setActions( actions)
 		        		    .setPriority(FlowModUtils.PRIORITY_MAX)
 		        		    .build());
 			    	
 			    	if (!res) {
	        			log.warn("ServiceMonitor:run() => Can not add flow entry for response flow for static LB.");
	        		}
 			    	
 			    	// log.info( "ServiceMonitor:run() => Flow entry for response has been configured." );
	        		// log.info("ServiceMonitor:run() => Result: {}", res);
 		        }
 		        
 		        // Static => Dynamic
 		        else if ( 1 == bIsStatic && dblLoadImb > IMBALANCE_THRESHOLD_CEILING_VAL) {
 		        	log.info("ServiceMonitor:run() => Converting to dynamic LB now... IMBALANCE value:{}", dblLoadImb);
 		        	
 		        	bIsStatic = 0;
 		        	
				// First, delete the flow entry which forwards the client request packets to the "select" group table.
	        		
				boolean res = ofSwitch.write(myFactory.buildFlowDelete()
 		        			.setMatch(myFactory.buildMatch()
 		        		    		.setExact(MatchField.ETH_TYPE, EthType.IPv4)
 		        		    		.setExact(MatchField.IP_PROTO, IpProtocol.TCP)
 		        		    		.setExact(MatchField.IPV4_DST, IPv4Address.of( VIP_ADDRESS ))
 		        		    		.setExact(MatchField.TCP_DST, TransportPort.of( SERVERHOST_SERVICE_PORT ) )
 		        		    		.build())
 		        			.build());
				
 			    	if (!res) {
	        			log.warn("ServiceMonitor:run() => Can not delete flow entry for forwarding to SELECT Group Table for static LB.");
	        		}
				
 		        	//  Delete the "SELECT" group Table
 		        	res = ofSwitch.write(myFactory.buildGroupDelete().setGroup(OFGroup.of(SELECT_GROUPTABLE_ID)).setGroupType(OFGroupType.SELECT).build() );
 		        	
 		        	if (!res) {
	        			log.warn("ServiceMonitor:run() => Can not delete SELECT group table for dynamic LB.");
	        		}
 		        	
 		        	//  Delete the response flow entry configured in the static LB mode.
 		        	res = ofSwitch.write(myFactory.buildFlowDelete()
 		        			.setMatch(myFactory.buildMatch()
 		        					.setExact(MatchField.ETH_TYPE, EthType.IPv4)
 		        					.setExact(MatchField.IP_PROTO, IpProtocol.TCP)
 		        					.setExact(MatchField.IPV4_DST, IPv4Address.of( CLIENT_ADDRESS ))
 		        					.setExact(MatchField.TCP_SRC, TransportPort.of( SERVERHOST_SERVICE_PORT ))
 		        					.build())
 		        			.build());
 		        	
 		        	if (!res) {
	        			log.warn("ServiceMonitor:run() => Can not delete flow entry for response flow for dynamic LB.");
	        		}
 		        }
 		        
 		        
 		        try {
 					Thread.sleep(MONITOR_INTERVAL);
 				} catch (InterruptedException e) {
 					// TODO Auto-generated catch block
 					e.printStackTrace();
 				}
 			}
 		}
 		
 		// nCnt is always SERVER_CNT. It includes the servers that already has broken.
 		double GetLoadImbalanceVal(double[] dblArrDynWeight, int nCnt) {
 			int i;
 			double dblAvgWeight = 0, dblLdImb = 0;
 			
 			for (i = 0; i < nCnt; i ++) {
 				dblAvgWeight += dblArrDynWeight[i];
 			}
 			dblAvgWeight /= nCnt;
 			
 			for (i = 0; i < nCnt; i ++) {
 				dblLdImb += (dblAvgWeight - dblArrDynWeight[i]) * (dblAvgWeight - dblArrDynWeight[i]);
 			}
 			
 			return dblLdImb;
 		}
 		
 		public String doGet(String URL){
 	        HttpURLConnection conn = null;
 	        InputStream is = null;
 	        BufferedReader br = null;
 	        StringBuilder result = new StringBuilder();
 	        try{
 	            URL url = new URL(URL);
 	            conn = (HttpURLConnection) url.openConnection();
 	            conn.setRequestMethod("GET");
 	            conn.setConnectTimeout(15000);
 	            conn.setReadTimeout(60000);
 	            conn.setRequestProperty("Accept", "application/json");
 	            conn.connect();
 	            
 	            if (200 == conn.getResponseCode()){
 	                is = conn.getInputStream();
 	                br = new BufferedReader(new InputStreamReader(is, "UTF-8"));
 	                String line;
 	                while ((line = br.readLine()) != null){
 	                    result.append(line);
 	                    // System.out.println(line);
 	                }
 	                
 	                // System.out.println("Result:" + result.toString());
 	                
 	            }else{
 	                System.out.println("ResponseCode is an error code:" + conn.getResponseCode());
 	            }
 	        }catch (MalformedURLException e){
 	            e.printStackTrace();
 	        }catch (IOException e){
 	            e.printStackTrace();
 	        }catch (Exception e){
 	            e.printStackTrace();
 	        }finally {
 	            try{
 	                if(br != null){
 	                    br.close();
 	                }
 	                if(is != null){
 	                    is.close();
 	                }
 	            }catch (IOException ioe){
 	                ioe.printStackTrace();
 	            }
 	            conn.disconnect();
 	        }
 	        return result.toString();
 	    }
 		
 	}
	public ServerMonitoring threadServerMonitoring;
	
	@Override
	public String getName() {
		return "loadbalancer";
	}

	@Override
	public boolean isCallbackOrderingPrereq(OFType type, String name) {
		return (type.equals(OFType.PACKET_IN) && 
				(name.equals("topology") || 
						name.equals("devicemanager") ||
						name.equals("virtualizer")));
	}

	@Override
	public boolean isCallbackOrderingPostreq(OFType type, String name) {
		return (type.equals(OFType.PACKET_IN) && name.equals("forwarding"));
	}

	@Override
	public net.floodlightcontroller.core.IListener.Command
	receive(IOFSwitch sw, OFMessage msg, FloodlightContext cntx) {
		switch (msg.getType()) {
		case PACKET_IN:
			return processPacketIn(sw, (OFPacketIn)msg, cntx);
		default:
			break;
		}
		log.warn("Received unexpected message {}", msg);
		return Command.CONTINUE;
	}

	private net.floodlightcontroller.core.IListener.Command processPacketIn(IOFSwitch sw, OFPacketIn pi, FloodlightContext cntx) {

		Ethernet eth = IFloodlightProviderService.bcStore.get(cntx, IFloodlightProviderService.CONTEXT_PI_PAYLOAD);
		IPacket pkt = eth.getPayload(); 	

		if (eth.isBroadcast() || eth.isMulticast()) {
			// handle ARP for VIP
			if (pkt instanceof ARP) {
				// retrieve arp to determine target IP address                                                       
				ARP arpRequest = (ARP) eth.getPayload();

				IPv4Address targetProtocolAddress = arpRequest.getTargetProtocolAddress();

				if (vipIpToId.containsKey(targetProtocolAddress.getInt())) {
					String vipId = vipIpToId.get(targetProtocolAddress.getInt());
					vipProxyArpReply(sw, pi, cntx, vipId);
					return Command.STOP;
				}
			}
		} else {
			// currently only load balance IPv4 packets - no-op for other traffic 
			if (pkt instanceof IPv4) {
				IPv4 ip_pkt = (IPv4) pkt;

				// If match Vip and port, check pool and choose member
				int destIpAddress = ip_pkt.getDestinationAddress().getInt();

				if (vipIpToId.containsKey(destIpAddress)){
					
					IPClient client = new IPClient();
					client.ipAddress = ip_pkt.getSourceAddress();
					client.nw_proto = ip_pkt.getProtocol();
					if (ip_pkt.getPayload() instanceof TCP) {
						TCP tcp_pkt = (TCP) ip_pkt.getPayload();

						client.srcPort = tcp_pkt.getSourcePort();
						client.targetPort = tcp_pkt.getDestinationPort();
					}
					if (ip_pkt.getPayload() instanceof UDP) {
						UDP udp_pkt = (UDP) ip_pkt.getPayload();
						client.srcPort = udp_pkt.getSourcePort();
						client.targetPort = udp_pkt.getDestinationPort();
					}
					if (ip_pkt.getPayload() instanceof ICMP) {
						client.srcPort = TransportPort.of(8); 
						client.targetPort = TransportPort.of(0); 
					}

					LBVip vip = vips.get(vipIpToId.get(destIpAddress));
					if (vip == null)			// fix dereference violations           
						return Command.CONTINUE;
					LBPool pool = pools.get(vip.pickPool(client));
					if (pool == null)			// fix dereference violations
						return Command.CONTINUE;

					HashMap<String, Short> memberWeights = new HashMap<String, Short>();
					HashMap<String, U64> memberPortBandwidth = new HashMap<String, U64>();
					
					if(pool.lbMethod == LBPool.WEIGHTED_RR){
						for(String memberId: pool.members){
							memberWeights.put(memberId,members.get(memberId).weight);
						}
					}
					// Switch statistics collection
					if(pool.lbMethod == LBPool.STATISTICS && statisticsService != null)
						memberPortBandwidth = collectSwitchPortBandwidth();
					
					LBMember member = members.get(pool.pickMember(client,memberPortBandwidth,memberWeights));
					if(member == null)			//fix dereference violations
						return Command.CONTINUE;

					
					// ROC
					String strCurLBMethod = "NULL";
					if (pool.lbMethod == LBPool.ROUND_ROBIN) strCurLBMethod = "ROUND_ROBIN";
					else if (pool.lbMethod == LBPool.STATISTICS) strCurLBMethod = "STATISTICS";
					else if (pool.lbMethod == LBPool.WEIGHTED_RR) strCurLBMethod = "WEIGHTED_RR";
					else if (pool.lbMethod == LBPool.DWRS_BINSEARCH) strCurLBMethod = "DWRS_BINSEARCH";
					
					log.info( "LoadBalancer:processPacketIn() => Current LB Method: {}, pickedMember ID: {} (address: {})", new Object[]{strCurLBMethod, member.id, member.address} );
					
					// for chosen member, check device manager and find and push routes, in both directions                    
					pushBidirectionalVipRoutes(sw, pi, cntx, client, member);
					
					// packet out based on table rule
					pushPacket(pkt, sw, pi.getBufferId(), (pi.getVersion().compareTo(OFVersion.OF_12) < 0) ? pi.getInPort() : pi.getMatch().get(MatchField.IN_PORT), OFPort.TABLE,
							cntx, true);


					return Command.STOP;
				}
			}
		}
		// bypass non-load-balanced traffic for normal processing (forwarding)
		return Command.CONTINUE;
	}

	/**
	 * used to collect statistics from members switch port
	 * @return HashMap<String, U64> portBandwidth <memberId,bitsPerSecond RX> of port connected to member
	 */
	public HashMap<String, U64> collectSwitchPortBandwidth(){
		HashMap<String,U64> memberPortBandwidth = new HashMap<String, U64>();
		HashMap<IDevice,String> deviceToMemberId = new HashMap<IDevice, String>();

		// retrieve all known devices to know which ones are attached to the members
		Collection<? extends IDevice> allDevices = deviceManagerService.getAllDevices();

		for (IDevice d : allDevices) {
			for (int j = 0; j < d.getIPv4Addresses().length; j++) {
				if(members != null){
					for(LBMember member: members.values()){
						if (member.address == d.getIPv4Addresses()[j].getInt())
							deviceToMemberId.put(d, member.id);
					}
				}
			}
		}
		// collect statistics of the switch ports attached to the members
		if(deviceToMemberId !=null){
			for(IDevice membersDevice: deviceToMemberId.keySet()){
				String memberId = deviceToMemberId.get(membersDevice);
				for(SwitchPort dstDap: membersDevice.getAttachmentPoints()){					
					SwitchPortBandwidth bandwidthOfPort = statisticsService.getBandwidthConsumption(dstDap.getNodeId(), dstDap.getPortId());
					if(bandwidthOfPort != null) // needs time for 1st collection, this avoids nullPointerException 
						memberPortBandwidth.put(memberId, bandwidthOfPort.getBitsPerSecondRx());
				}
			}
		}
		return memberPortBandwidth;
	}

	/**
	 * used to send proxy Arp for load balanced service requests
	 * @param IOFSwitch sw
	 * @param OFPacketIn pi
	 * @param FloodlightContext cntx
	 * @param String vipId
	 */

	protected void vipProxyArpReply(IOFSwitch sw, OFPacketIn pi, FloodlightContext cntx, String vipId) {
		log.debug("vipProxyArpReply");

		Ethernet eth = IFloodlightProviderService.bcStore.get(cntx,
				IFloodlightProviderService.CONTEXT_PI_PAYLOAD);

		// retrieve original arp to determine host configured gw IP address                                          
		if (! (eth.getPayload() instanceof ARP))
			return;
		ARP arpRequest = (ARP) eth.getPayload();

		// have to do proxy arp reply since at this point we cannot determine the requesting application type

		// generate proxy ARP reply
		IPacket arpReply = new Ethernet()
				.setSourceMACAddress(vips.get(vipId).proxyMac)
				.setDestinationMACAddress(eth.getSourceMACAddress())
				.setEtherType(EthType.ARP)
				.setVlanID(eth.getVlanID())
				.setPriorityCode(eth.getPriorityCode())
				.setPayload(
						new ARP()
						.setHardwareType(ARP.HW_TYPE_ETHERNET)
						.setProtocolType(ARP.PROTO_TYPE_IP)
						.setHardwareAddressLength((byte) 6)
						.setProtocolAddressLength((byte) 4)
						.setOpCode(ARP.OP_REPLY)
						.setSenderHardwareAddress(vips.get(vipId).proxyMac)
						.setSenderProtocolAddress(arpRequest.getTargetProtocolAddress())
						.setTargetHardwareAddress(eth.getSourceMACAddress())
						.setTargetProtocolAddress(arpRequest.getSenderProtocolAddress()));

		// push ARP reply out
		pushPacket(arpReply, sw, OFBufferId.NO_BUFFER, OFPort.ANY, (pi.getVersion().compareTo(OFVersion.OF_12) < 0 ? pi.getInPort() : pi.getMatch().get(MatchField.IN_PORT)), cntx, true);
		log.debug("proxy ARP reply pushed as {}", IPv4.fromIPv4Address(vips.get(vipId).address));

		return;
	}

	/**
	 * used to push any packet - borrowed routine from Forwarding
	 * 
	 * @param OFPacketIn pi
	 * @param IOFSwitch sw
	 * @param int bufferId
	 * @param short inPort
	 * @param short outPort
	 * @param FloodlightContext cntx
	 * @param boolean flush
	 */    
	public void pushPacket(IPacket packet, 
			IOFSwitch sw,
			OFBufferId bufferId,
			OFPort inPort,
			OFPort outPort, 
			FloodlightContext cntx,
			boolean flush) {
		if (log.isTraceEnabled()) {
			log.trace("PacketOut srcSwitch={} inPort={} outPort={}", 
					new Object[] {sw, inPort, outPort});
		}

		OFPacketOut.Builder pob = sw.getOFFactory().buildPacketOut();

		// set actions
		List<OFAction> actions = new ArrayList<OFAction>();
		actions.add(sw.getOFFactory().actions().buildOutput().setPort(outPort).setMaxLen(Integer.MAX_VALUE).build());

		pob.setActions(actions);

		// set buffer_id, in_port
		pob.setBufferId(bufferId);
		OFMessageUtils.setInPort(pob, inPort);

		// set data - only if buffer_id == -1
		if (pob.getBufferId() == OFBufferId.NO_BUFFER) {
			if (packet == null) {
				log.error("BufferId is not set and packet data is null. " +
						"Cannot send packetOut. " +
						"srcSwitch={} inPort={} outPort={}",
						new Object[] {sw, inPort, outPort});
				return;
			}
			byte[] packetData = packet.serialize();
			pob.setData(packetData);
		}


		counterPacketOut.increment();
		sw.write(pob.build());

	}

	/**
	 * used to find and push in-bound and out-bound routes using StaticFlowEntryPusher
	 * @param IOFSwitch sw
	 * @param OFPacketIn pi
	 * @param FloodlightContext cntx
	 * @param IPClient client
	 * @param LBMember member
	 */
	protected void pushBidirectionalVipRoutes(IOFSwitch sw, OFPacketIn pi, FloodlightContext cntx, IPClient client, LBMember member) {

		// borrowed code from Forwarding to retrieve src and dst device entities
		// Check if we have the location of the destination
		IDevice srcDevice = null;
		IDevice dstDevice = null;

		// retrieve all known devices
		Collection<? extends IDevice> allDevices = deviceManagerService.getAllDevices();

		for (IDevice d : allDevices) {
			for (int j = 0; j < d.getIPv4Addresses().length; j++) {
				if (srcDevice == null && client.ipAddress.equals(d.getIPv4Addresses()[j]))
					srcDevice = d;
				if (dstDevice == null && member.address == d.getIPv4Addresses()[j].getInt()) {
					dstDevice = d;
					member.macString = dstDevice.getMACAddressString();
				}
				if (srcDevice != null && dstDevice != null)
					break;
			}
		}

		// srcDevice and/or dstDevice is null, no route can be pushed
		if (srcDevice == null || dstDevice == null) return;

		DatapathId srcIsland = topologyService.getClusterId(sw.getId());

		if (srcIsland == null) {
			log.debug("No openflow island found for source {}/{}", 
					sw.getId().toString(), pi.getInPort());
			return;
		}

		// Validate that we have a destination known on the same island
		// Validate that the source and destination are not on the same switchport
		boolean on_same_island = false;
		boolean on_same_if = false;
		//Switch
		for (SwitchPort dstDap : dstDevice.getAttachmentPoints()) {
			DatapathId dstSwDpid = dstDap.getNodeId();
			DatapathId dstIsland = topologyService.getClusterId(dstSwDpid);
			if ((dstIsland != null) && dstIsland.equals(srcIsland)) {
				on_same_island = true;
				if ((sw.getId().equals(dstSwDpid)) && OFMessageUtils.getInPort(pi).equals(dstDap.getPortId())) {
					on_same_if = true;
				}
				break;
			}
		}

		if (!on_same_island) {
			// Flood since we don't know the dst device
			if (log.isTraceEnabled()) {
				log.trace("No first hop island found for destination " + 
						"device {}, Action = flooding", dstDevice);
			}
			return;
		}            

		if (on_same_if) {
			if (log.isTraceEnabled()) {
				log.trace("Both source and destination are on the same " + 
						"switch/port {}/{}, Action = NOP", 
						sw.toString(), pi.getInPort());
			}
			return;
		}	

		// Install all the routes where both src and dst have attachment
		// points.  Since the lists are stored in sorted order we can 
		// traverse the attachment points in O(m+n) time
		SwitchPort[] srcDaps = srcDevice.getAttachmentPoints();
		Arrays.sort(srcDaps, clusterIdComparator);
		SwitchPort[] dstDaps = dstDevice.getAttachmentPoints();
		Arrays.sort(dstDaps, clusterIdComparator);

		int iSrcDaps = 0, iDstDaps = 0;

		// following Forwarding's same routing routine, retrieve both in-bound and out-bound routes for
		// all clusters.
		while ((iSrcDaps < srcDaps.length) && (iDstDaps < dstDaps.length)) {
			SwitchPort srcDap = srcDaps[iSrcDaps];
			SwitchPort dstDap = dstDaps[iDstDaps];
			DatapathId srcCluster = 
					topologyService.getClusterId(srcDap.getNodeId());
			DatapathId dstCluster = 
					topologyService.getClusterId(dstDap.getNodeId());

			int srcVsDest = srcCluster.compareTo(dstCluster);
			if (srcVsDest == 0) {
				if (!srcDap.equals(dstDap) && 
						(srcCluster != null) && 
						(dstCluster != null)) {
					Path routeIn = 
							routingEngineService.getPath(srcDap.getNodeId(),
									srcDap.getPortId(),
									dstDap.getNodeId(),
									dstDap.getPortId());
					Path routeOut = 
							routingEngineService.getPath(dstDap.getNodeId(),
									dstDap.getPortId(),
									srcDap.getNodeId(),
									srcDap.getPortId());

					// use static flow entry pusher to push flow mod along in and out path
					// in: match src client (ip, port), rewrite dest from vip ip/port to member ip/port, forward
					// out: match dest client (ip, port), rewrite src from member ip/port to vip ip/port, forward

					if (! routeIn.getPath().isEmpty()) {
						pushStaticVipRoute(true, routeIn, client, member, sw);
					}

					if (! routeOut.getPath().isEmpty()) {
						pushStaticVipRoute(false, routeOut, client, member, sw);
					}

				}
				iSrcDaps++;
				iDstDaps++;
			} else if (srcVsDest < 0) {
				iSrcDaps++;
			} else {
				iDstDaps++;
			}
		}
		return;
	}

	/**
	 * used to push given route using static flow entry pusher
	 * @param boolean inBound
	 * @param Path route
	 * @param IPClient client
	 * @param LBMember member
	 * @param long pinSwitch
	 */
	public void pushStaticVipRoute(boolean inBound, Path route, IPClient client, LBMember member, IOFSwitch pinSwitch) {

		List<NodePortTuple> path = route.getPath();
		if (path.size() > 0) {
			for (int i = 0; i < path.size(); i+=2) {
				DatapathId sw = path.get(i).getNodeId();
				String entryName;
				Match.Builder mb = pinSwitch.getOFFactory().buildMatch();
				ArrayList<OFAction> actions = new ArrayList<OFAction>();

				OFFlowMod.Builder fmb = pinSwitch.getOFFactory().buildFlowAdd();

				fmb.setIdleTimeout(FlowModUtils.INFINITE_TIMEOUT);
				fmb.setHardTimeout(FlowModUtils.INFINITE_TIMEOUT);
				fmb.setBufferId(OFBufferId.NO_BUFFER);
				fmb.setOutPort(OFPort.ANY);
				fmb.setCookie(U64.of(0));  
				fmb.setPriority(FlowModUtils.PRIORITY_MAX);


				if (inBound) {
					entryName = "inbound-vip-"+ member.vipId+"-client-"+client.ipAddress
							+"-srcport-"+client.srcPort+"-dstport-"+client.targetPort
							+"-srcswitch-"+path.get(0).getNodeId()+"-sw-"+sw;
					mb.setExact(MatchField.ETH_TYPE, EthType.IPv4)
					.setExact(MatchField.IP_PROTO, client.nw_proto)
					.setExact(MatchField.IPV4_SRC, client.ipAddress)
					.setExact(MatchField.IN_PORT, path.get(i).getPortId());
					
					if (client.nw_proto.equals(IpProtocol.TCP)) {
						mb.setExact(MatchField.TCP_SRC, client.srcPort);
					} else if (client.nw_proto.equals(IpProtocol.UDP)) {
						mb.setExact(MatchField.UDP_SRC, client.srcPort);
					} else if (client.nw_proto.equals(IpProtocol.SCTP)) {
						mb.setExact(MatchField.SCTP_SRC, client.srcPort);
					} else if (client.nw_proto.equals(IpProtocol.ICMP)) {
						/* no-op */
					} else {
						log.error("Unknown IpProtocol {} detected during inbound static VIP route push.", client.nw_proto);
					}
					
					if (sw.equals(pinSwitch.getId())) {
						
						// ROC
						// if lbMethod is "DWRS_BINSEARCH" and dstIPAddress "VIP_ADDRESS",  
						// we set the actions as the goto action to the ff group table.
						
						// log.info("LBMethod: {}, clientn.address: {}, client.targetPort: {}", 
						//		new Object[]{pools.get(member.poolId).lbMethod, client.ipAddress.toString(), client.targetPort.toString()} );
						
						if (pools.get(member.poolId).lbMethod == LBPool.DWRS_BINSEARCH
								&& client.targetPort.compareTo( TransportPort.of(SERVERHOST_SERVICE_PORT) ) ==  0) {
							
							// get the server id from member.id. 
							int nServerId = Integer.valueOf(member.id) - 1;
							
							// set the action as the goto action to the ff group table.
							actions.add( pinSwitch.getOFFactory().actions().buildGroup().
					        	    		setGroup( OFGroup.of(nArrFailoverGroupTableID[nServerId])).build() );
				        	
							// log.info("LoadBalancer:pushStaticVipRoute() => goto action to the GROUP TABLE[{}] is added to the actions.", nArrFailoverGroupTableID[nServerId]);
						}
						
						// otherwise, create the default set_field action.
						else {
							if (pinSwitch.getOFFactory().getVersion().compareTo(OFVersion.OF_12) < 0) {
								actions.add(pinSwitch.getOFFactory().actions().setDlDst(MacAddress.of(member.macString)));
								actions.add(pinSwitch.getOFFactory().actions().setNwDst(IPv4Address.of(member.address)));
								actions.add(pinSwitch.getOFFactory().actions().output(path.get(i+1).getPortId(), Integer.MAX_VALUE));
							} else { // OXM introduced in OF1.2
								actions.add(pinSwitch.getOFFactory().actions().setField(pinSwitch.getOFFactory().oxms().ethDst(MacAddress.of(member.macString))));
								actions.add(pinSwitch.getOFFactory().actions().setField(pinSwitch.getOFFactory().oxms().ipv4Dst(IPv4Address.of(member.address))));
								actions.add(pinSwitch.getOFFactory().actions().output(path.get(i+1).getPortId(), Integer.MAX_VALUE));
							}
						}
					} else {
						//fix concurrency errors
						try{
							actions.add(switchService.getSwitch(path.get(i+1).getNodeId()).getOFFactory().actions().output(path.get(i+1).getPortId(), Integer.MAX_VALUE));
						}
						catch(NullPointerException e){
							log.error("Fail to install loadbalancer flow rules to offline switch {}.", path.get(i+1).getNodeId());
						}
					}
				} else {
					entryName = "outbound-vip-"+ member.vipId+"-client-"+client.ipAddress
							+"-srcport-"+client.srcPort+"-dstport-"+client.targetPort
							+"-srcswitch-"+path.get(0).getNodeId()+"-sw-"+sw;
					mb.setExact(MatchField.ETH_TYPE, EthType.IPv4)
					.setExact(MatchField.IP_PROTO, client.nw_proto)
					.setExact(MatchField.IPV4_DST, client.ipAddress)
					.setExact(MatchField.IN_PORT, path.get(i).getPortId());
					if (client.nw_proto.equals(IpProtocol.TCP)) {
						mb.setExact(MatchField.TCP_DST, client.srcPort);
					} else if (client.nw_proto.equals(IpProtocol.UDP)) {
						mb.setExact(MatchField.UDP_DST, client.srcPort);
					} else if (client.nw_proto.equals(IpProtocol.SCTP)) {
						mb.setExact(MatchField.SCTP_DST, client.srcPort);
					} else if (client.nw_proto.equals(IpProtocol.ICMP)) {
						/* no-op */
					} else {
						log.error("Unknown IpProtocol {} detected during outbound static VIP route push.", client.nw_proto);
					}

					if (sw.equals(pinSwitch.getId())) {
						if (pinSwitch.getOFFactory().getVersion().compareTo(OFVersion.OF_12) < 0) { 
							actions.add(pinSwitch.getOFFactory().actions().setDlSrc(vips.get(member.vipId).proxyMac));
							actions.add(pinSwitch.getOFFactory().actions().setNwSrc(IPv4Address.of(vips.get(member.vipId).address)));
							actions.add(pinSwitch.getOFFactory().actions().output(path.get(i+1).getPortId(), Integer.MAX_VALUE));
						} else { // OXM introduced in OF1.2
							actions.add(pinSwitch.getOFFactory().actions().setField(pinSwitch.getOFFactory().oxms().ethSrc(vips.get(member.vipId).proxyMac)));
							actions.add(pinSwitch.getOFFactory().actions().setField(pinSwitch.getOFFactory().oxms().ipv4Src(IPv4Address.of(vips.get(member.vipId).address))));
							actions.add(pinSwitch.getOFFactory().actions().output(path.get(i+1).getPortId(), Integer.MAX_VALUE));

						}
					} else {
						//fix concurrency errors
						try{
							actions.add(switchService.getSwitch(path.get(i+1).getNodeId()).getOFFactory().actions().output(path.get(i+1).getPortId(), Integer.MAX_VALUE));
						}
						catch(NullPointerException e){
							log.error("Fail to install loadbalancer flow rules to offline switches {}.", path.get(i+1).getNodeId());
						}
					}

				}


				fmb.setActions(actions);
				fmb.setPriority(U16.t(LB_PRIORITY));
				fmb.setMatch(mb.build());
				sfpService.addFlow(entryName, fmb.build(), sw);
			}
		}

		return;
	}

	@Override
	public Collection<LBVip> listVips() {
		return vips.values();
	}

	@Override
	public Collection<LBVip> listVip(String vipId) {
		Collection<LBVip> result = new HashSet<LBVip>();
		result.add(vips.get(vipId));
		return result;
	}

	@Override
	public LBVip createVip(LBVip vip) {
		if (vip == null)
			vip = new LBVip();

		vips.put(vip.id, vip);
		vipIpToId.put(vip.address, vip.id);
		vipIpToMac.put(vip.address, vip.proxyMac);

		return vip;
	}

	@Override
	public LBVip updateVip(LBVip vip) {
		vips.put(vip.id, vip);
		return vip;
	}

	@Override
	public int removeVip(String vipId) {
		if(vips.containsKey(vipId)){
			vips.remove(vipId);
			return 0;
		} else {
			return -1;
		}
	}

	@Override
	public Collection<LBPool> listPools() {
		return pools.values();
	}

	@Override
	public Collection<LBPool> listPool(String poolId) {
		Collection<LBPool> result = new HashSet<LBPool>();
		result.add(pools.get(poolId));
		return result;
	}

	@Override
	public LBPool createPool(LBPool pool) {
		if (pool == null)
			pool = new LBPool();

		pools.put(pool.id, pool);
		if (pool.vipId != null && vips.containsKey(pool.vipId))
			vips.get(pool.vipId).pools.add(pool.id);
		else {
			log.error("specified vip-id must exist");
			pool.vipId = null;
			pools.put(pool.id, pool);
		}
		return pool;
	}

	@Override
	public LBPool updatePool(LBPool pool) {
		pools.put(pool.id, pool);
		return null;
	}

	@Override
	public int removePool(String poolId) {
		LBPool pool;
		if (pools != null) {
			pool = pools.get(poolId);
			if (pool == null)	// fix dereference violations
				return -1;
			if (pool.vipId != null && vips.containsKey(pool.vipId))
				vips.get(pool.vipId).pools.remove(poolId);
			pools.remove(poolId);
			return 0;
		} else {
			return -1;
		}
	}

	@Override
	public Collection<LBMember> listMembers() {
		return members.values();
	}

	@Override
	public Collection<LBMember> listMember(String memberId) {
		Collection<LBMember> result = new HashSet<LBMember>();
		result.add(members.get(memberId));
		return result;
	}

	@Override
	public Collection<LBMember> listMembersByPool(String poolId) {
		Collection<LBMember> result = new HashSet<LBMember>();

		if(pools.containsKey(poolId)) {
			ArrayList<String> memberIds = pools.get(poolId).members;
			if(memberIds !=null && members != null){
				for (int i = 0; i<memberIds.size(); i++)
					result.add(members.get(memberIds.get(i)));
			}
		}
		return result;
	}

	@Override
	public LBMember createMember(LBMember member) {
		if (member == null)
			member = new LBMember();

		members.put(member.id, member);
		memberIpToId.put(member.address, member.id);

		if (member.poolId != null && pools.get(member.poolId) != null) {
			member.vipId = pools.get(member.poolId).vipId;
			if (!pools.get(member.poolId).members.contains(member.id))
				pools.get(member.poolId).members.add(member.id);
		} else
			log.error("member must be specified with non-null pool_id");

		return member;
	}

	@Override
	public LBMember updateMember(LBMember member) {
		members.put(member.id, member);
		return member;
	}

	@Override
	public int removeMember(String memberId) {
		LBMember member;
		member = members.get(memberId);

		if(member != null){
			if (member.poolId != null && pools.containsKey(member.poolId))
				pools.get(member.poolId).members.remove(memberId);
			members.remove(memberId);
			return 0;
		} else {
			return -1;
		}    
	}

	@Override
	public int setMemberWeight(String memberId, String weight){
		LBMember member;
		short value;
		member = members.get(memberId);

		try{
			value = Short.parseShort(weight);
		} catch(Exception e){
			log.error("Invalid value for member weight " + e.getMessage());
			return -1;
		}
		if(member != null && (value <= 10 && value >= 1)){
			member.weight = value;
			return 0;
		}
		return -1;
	}

	public int setPriorityToMember(String poolId ,String memberId){
		if(pools.containsKey(poolId)) {
			ArrayList<String> memberIds = pools.get(poolId).members;
			if(memberIds !=null && members != null && memberIds.contains(memberId)){
				for (int i = 0; i<memberIds.size(); i++){
					if(members.get(memberIds.get(i)).id.equals(memberId)){
						members.get(memberId).weight=(short) (1 + memberIds.size()/2);
					}else
						members.get(memberIds.get(i)).weight=1;
				}
				return 0;
			}
		}
		return -1;
	}
	@Override
	public Collection<LBMonitor> listMonitors() {
		return null;
	}

	@Override
	public Collection<LBMonitor> listMonitor(String monitorId) {
		return null;
	}

	@Override
	public LBMonitor createMonitor(LBMonitor monitor) {
		return null;
	}

	@Override
	public LBMonitor updateMonitor(LBMonitor monitor) {
		return null;
	}

	@Override
	public int removeMonitor(String monitorId) {
		return 0;
	}

	@Override
	public Collection<Class<? extends IFloodlightService>>
	getModuleServices() {
		Collection<Class<? extends IFloodlightService>> l = 
				new ArrayList<Class<? extends IFloodlightService>>();
		l.add(ILoadBalancerService.class);
		return l;
	}

	@Override
	public Map<Class<? extends IFloodlightService>, IFloodlightService>
	getServiceImpls() {
		Map<Class<? extends IFloodlightService>, IFloodlightService> m = 
				new HashMap<Class<? extends IFloodlightService>,
				IFloodlightService>();
		m.put(ILoadBalancerService.class, this);
		return m;
	}

	@Override
	public Collection<Class<? extends IFloodlightService>>
	getModuleDependencies() {
		Collection<Class<? extends IFloodlightService>> l = 
				new ArrayList<Class<? extends IFloodlightService>>();
		l.add(IFloodlightProviderService.class);
		l.add(IRestApiService.class);
		l.add(IOFSwitchService.class);
		l.add(IDeviceService.class);
		l.add(IDebugCounterService.class);
		l.add(ITopologyService.class);
		l.add(IRoutingService.class);
		l.add(IStaticEntryPusherService.class);
		l.add(IStatisticsService.class);

		return l;
	}

	@Override
	public void init(FloodlightModuleContext context)
			throws FloodlightModuleException {
		floodlightProviderService = context.getServiceImpl(IFloodlightProviderService.class);
		restApiService = context.getServiceImpl(IRestApiService.class);
		debugCounterService = context.getServiceImpl(IDebugCounterService.class);
		deviceManagerService = context.getServiceImpl(IDeviceService.class);
		routingEngineService = context.getServiceImpl(IRoutingService.class);
		topologyService = context.getServiceImpl(ITopologyService.class);
		sfpService = context.getServiceImpl(IStaticEntryPusherService.class);
		switchService = context.getServiceImpl(IOFSwitchService.class);
		statisticsService = context.getServiceImpl(IStatisticsService.class);

		vips = new HashMap<String, LBVip>();
		pools = new HashMap<String, LBPool>();
		members = new HashMap<String, LBMember>();
		vipIpToId = new HashMap<Integer, String>();
		vipIpToMac = new HashMap<Integer, MacAddress>();
		memberIpToId = new HashMap<Integer, String>();
	}

	@Override
	public void startUp(FloodlightModuleContext context) {
		log.info("LoadBalancer:startUp() => Load Balancer is Starting up...");
    	
    	// ROC START
    	
    	////////////////// Create VIP
    	{
    		log.info("LoadBalancer:startUp() => Creating VIP");
    		
	    	LBVip vip1 = null;
	    	String postData1;
	    	VipsResource vipsResource = new VipsResource();
			IOException error = null;
	
			postData1 = "{\"id\":\"1\",\"name\":\"vip1\",\"protocol\":\"icmp\",\"address\":\"192.168.198.160\",\"port\":\"8\"}";
			
	
			try {
				vip1 = vipsResource.jsonToVip(postData1);
			} catch (IOException e) {
				error = e;
			}
			
	
			// verify correct parsing
			// assertFalse(vip1==null);
			// assertTrue(error==null);
	
			createVip(vip1);

			log.info("LoadBalancer:startUp() => vip1: {}", vip1.toString());
			// verify correct creation
			// assertTrue(vips.containsKey(vip1.id));
	    }
		
		//////////////////Create Pool
    	{
    		log.info("LoadBalancer:startUp() => Creating pool");
    		
    		LBPool pool1 = null;
			String postData1;
			PoolsResource poolsResource = new PoolsResource();
			IOException error = null;
			
			postData1 = "{\"id\":\"1\",\"name\":\"pool1\",\"protocol\":\"icmp\",\"vip_id\":\"1\"}";
	
			try {
				pool1 = poolsResource.jsonToPool(postData1);
			} catch (IOException e) {
				error = e;
			}
	
			// verify correct parsing
			// assertFalse(pool1==null);
			// assertTrue(error==null);
	
			createPool(pool1);

			pool1.lbMethod = LBPool.DWRS_BINSEARCH;
			pool1.nServerCnt = SERVER_CNT;
			log.info("LoadBalancer:startUp() => pool1: {}", pool1.toString());
			
			// verify successful creates; two registered with vips and one not
			// assertTrue(pools.containsKey(pool1.id));
			// assertTrue(vips.get(pool1.vipId).pools.contains(pool1.id));
    	}
    	

    	//////////////////Create Member
    	{
    		log.info("LoadBalancer:startUp() => Creating member");
    		
    		LBMember[] members = new LBMember[8];
    		// LBMember member3 = null, member4 = null;
    		String[] postData = new String[8];
    		MembersResource membersResource = new MembersResource();
    		IOException error = null;
    		
    		postData[0] = "{\"id\":\"1\",\"address\":\"192.168.198.151\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		postData[1] = "{\"id\":\"2\",\"address\":\"192.168.198.152\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		postData[2] = "{\"id\":\"3\",\"address\":\"192.168.198.153\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		postData[3] = "{\"id\":\"4\",\"address\":\"192.168.198.154\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		postData[4] = "{\"id\":\"5\",\"address\":\"192.168.198.155\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		postData[5] = "{\"id\":\"6\",\"address\":\"192.168.198.156\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		postData[6] = "{\"id\":\"7\",\"address\":\"192.168.198.157\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		postData[7] = "{\"id\":\"8\",\"address\":\"192.168.198.158\",\"port\":\"9100\",\"pool_id\":\"1\"}";
    		
    		for (int i = 0; i < SERVER_CNT; i ++) {
        		try {
        			members[i] = membersResource.jsonToMember(postData[i]);
        			createMember(members[i]);
        			log.info("LoadBalancer:startUp() => member[{}]: {}", i, members[i].toString());
        		} catch (IOException e) {
        			error = e;
        		}
    		}
    		
    		// log.info( "IPv4Address.toString() test: {} (int: {})", IPv4Address.of(members[0].address).toString(), members[0].address);
    	}
		
    	{
    		String strCurLBMethod = "NULL";
			if (pools.get("1").lbMethod == LBPool.ROUND_ROBIN) strCurLBMethod = "ROUND_ROBIN";
			else if (pools.get("1").lbMethod == LBPool.STATISTICS) strCurLBMethod = "STATISTICS";
			else if (pools.get("1").lbMethod == LBPool.WEIGHTED_RR) strCurLBMethod = "WEIGHTED_RR";
			else if (pools.get("1").lbMethod == LBPool.DWRS_BINSEARCH) strCurLBMethod = "DWRS_BINSEARCH";
			else if (pools.get("1").lbMethod == LBPool.DWRS_LINEAR) strCurLBMethod = "DWRS_LINEAR";
			else if (pools.get("1").lbMethod == LBPool.LEAST_LOAD) strCurLBMethod = "LEAST_LOAD";
			
			log.info( "LoadBalancer:startUp() => Current LB Method: {}", strCurLBMethod );
			
			if (pools.get("1").lbMethod != LBPool.ROUND_ROBIN) {
			threadServerMonitoring = new ServerMonitoring(pools.get("1"), SWITCH_ID);
    		threadServerMonitoring.start();
			}
    	}
    	
		// ROC END
		
		floodlightProviderService.addOFMessageListener(OFType.PACKET_IN, this);
		restApiService.addRestletRoutable(new LoadBalancerWebRoutable());
		debugCounterService.registerModule(this.getName());
		counterPacketOut = debugCounterService.registerCounter(this.getName(), "packet-outs-written", "Packet outs written by the LoadBalancer", MetaData.WARN);
	}
}
