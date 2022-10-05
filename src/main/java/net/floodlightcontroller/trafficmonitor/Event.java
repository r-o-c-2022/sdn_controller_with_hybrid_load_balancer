package net.floodlightcontroller.trafficmonitor;

import java.util.Date;

import org.projectfloodlight.openflow.types.U64;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;

import net.floodlightcontroller.core.types.NodePortTuple;
import net.floodlightcontroller.trafficmonitor.web.EventSerializer;

@JsonSerialize(using=EventSerializer.class)
public class Event {
	private Date 				time;			
	private NodePortTuple 		source;			// �¼�������Դ
	private U64					rxSpeed;		// �˿���������
	private U64					txSpeed;		// �˿��������
	private Policy				policy;			// ϵͳִ�в���
	
	public Event(){
		time = new Date();
		source = null;
		rxSpeed = txSpeed = U64.ZERO;
		policy = null;
	}
	public Event(SwitchPortStatistics sps, Policy policy){
		time = new Date();
		source = new NodePortTuple(sps.getDpid(), sps.getPortNo());
		rxSpeed = U64.of(sps.getRxSpeed().getValue());
		txSpeed = U64.of(sps.getTxSpeed().getValue());
		this.policy = new Policy(policy);
	}
	
	
	public Date getTime() {
		return time;
	}

	public void setTime(Date time) {
		this.time = time;
	}

	public NodePortTuple getSource() {
		return source;
	}

	public void setSource(NodePortTuple source) {
		this.source = source;
	}

	public Policy getPolicy() {
		return policy;
	}

	public void setPolicy(Policy policy) {
		this.policy = policy;
	}

	public U64 getRxSpeed() {
		return rxSpeed;
	}

	public void setRxSpeed(U64 rxSpeed) {
		this.rxSpeed = rxSpeed;
	}

	public U64 getTxSpeed() {
		return txSpeed;
	}

	public void setTxSpeed(U64 txSpeed) {
		this.txSpeed = txSpeed;
	}
	
	public String description(){
		return "";
	}
	
	public String systemAction(){
		return "";
	}
	
}
