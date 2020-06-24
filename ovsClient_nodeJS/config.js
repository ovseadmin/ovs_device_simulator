module.exports = {
    // mqtt
    host: '223.39.121.186', //staging
    port: 1883,
    username: 'csx13123451234500001',
    password: 'csx13123451234500001',
    qos: 1,
    reconnectedPeriod: 1000,
    connectTimeout: 30 * 1000,

    // number of Tx msg regarding of location
    numOfTxMsg: 1000,        // 1000 sec. of trip time
    TxInterval: 1000,        // location interval in ms
    TxIntervalEvent: 10000,  // event interval in ms

    // Device Types : OVC-G, OVC-M
    // 코드로 바뀌면 수정해야 함
    deviceType : 'OVC-G',

    // gps 
    speed: 80,
 
}