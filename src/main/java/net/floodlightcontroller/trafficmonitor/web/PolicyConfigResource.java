package net.floodlightcontroller.trafficmonitor.web;

import java.util.HashMap;

import net.floodlightcontroller.trafficmonitor.ITrafficMonitorService;
import net.floodlightcontroller.trafficmonitor.Policy;

import org.projectfloodlight.openflow.types.U64;
import org.restlet.resource.Post;
import org.restlet.resource.Put;
import org.restlet.resource.ServerResource;

/**
 * ��������������󣬽�POST/PUT�����ĵ�JSON�ַ���ת������Ӧ�������͵Ĳ����������и�ֵ
 * 
 *
 */
public class PolicyConfigResource extends ServerResource {
	@Post
	@Put
	public String config(String json) {
		ITrafficMonitorService trafficMonitorService = (ITrafficMonitorService) getContext().getAttributes().get(ITrafficMonitorService.class.getCanonicalName());
		HashMap<String, String> jsonHashMap = jsonToHashMap(json);
		
		/* ȡjson�ֶΣ���ֵ����Ӧ���� */
		U64 portSpeedThreshold = U64.of(Long.valueOf(jsonHashMap.get("traffic_threshold")));
		String action = jsonHashMap.get("action");
		long actionDuration = Long.parseLong(jsonHashMap.get("action_duration"));
		U64 rateLimit = U64.of(Long.valueOf(jsonHashMap.get("rate_limit")));


		trafficMonitorService.setPolicy(portSpeedThreshold, action, actionDuration, rateLimit);
		return "{\"status\":\"Policy updated\"}";
	}
	
	/**
	 * ���ַ�����ʽ��jsonת����Hashmap��ʽ��field��value֮���ӳ�䣩
	 * @param �ַ�����ʽjson
	 * @return Hashmap��ʽjson
	 */
	private HashMap jsonToHashMap(String json){
		HashMap<String, String> result = new HashMap<>();
		String subString = removeBraces(json);

		String[] rows = subString.split(",");
		for(String row : rows){
			String[] columns = row.split(":");
			
			String key,value;
			key = columns[0].substring(1, columns[0].length()-1);
			value = columns[1].substring(1, columns[1].length()-1);
			
			result.put(key, value);
		}
		return result;
		
	}
	/**
	 * ȥ���ַ�����ָ��λ�õ�'{'��'}' 
	 * @param string
	 * @return ȥ�������ź��string
	 */
	private String removeBraces(String string){
		String result = null;
		
		if(string.charAt(0) == '{' && string.charAt(string.length()-1) == '}'){
			result = string.substring(1, string.length()-1);
		}
		
		return result;
	}
	
}
