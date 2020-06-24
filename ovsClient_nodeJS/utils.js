module.exports = {
    // topic
    // https://tde.sktelecom.com/wiki/pages/viewpage.action?pageId=281908705
    deviceTopic : 'ovs/device/',
    locationTopic : 'ovs/location',
    eventTopic : 'ovs/event',

    // event type
    // 201: 급정거발생이벤트
    // 202: 차량사고발생이벤트
    // 203: 졸음운전발생이벤트
    eventType : '201',

    // 수신 msg 중, type 파싱
    typeParsing : function(msg_type) {
        switch (msg_type){
            case 0:
                var et_str = '전방 급정거 발생';
                break
            case 258:
                var et_str = '전방 차량 정체';
                break;
            case 513:    
                var et_str = '전방 사고 발생';
                break
            case 534:    
                var et_str = '전방 정지차 주의';
                break
            case 1281:    
                var et_str = '전방 낙하물 주의';
                break
            case 1286:    
                var et_str = '전방 보행자 주의';
                break
            case 1793:   
                var et_str = '전방 차량 역주행 주의';
                break
            case 9731:    
                var et_str = '후방 경찰차 접근';
                break
            case 9734:    
                var et_str = '후방 구급차 접근';
                break
            case 9736:    
                var et_str = '후방 소방차 접근';
                break
        }
        console.log('event=',msg_type,', et_str=',et_str)
        return et_str;
    },

    // 수신 msg 중, tunnel 파싱
    tunnelParsing : function(msg_tn){
        switch (msg_tn){
            case true:
                var tn_str = '터널 내부';
                break
            case false:
                var tn_str = '터널 외부';
                break
        }
        return tn_str;
    },

    // 수신 msg 중, distanceToEvent 파싱
    distanceToEventParsing : function(msg_dte){
        if(msg_dte>0){
            var dte_str = msg_dte + 'm 앞에서';
            return dte_str;
        }else{
            var dte_str = msg_dte*(-1) + 'm 뒤에서';
            return dte_str;
        }
    }   



}