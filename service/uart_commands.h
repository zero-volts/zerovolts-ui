//-- UART COMMANDS -- //

#define BT_COMMAND_REQ_SCANN        "SCAN"          // el usuario comienza el scaneo
#define BT_COMMAND_RES_SCANN_START  "SCAN:START"    // el esp32 avisa que comenzo a scanear dispositivos
#define BT_COMMAND_RES_SCANN_DONE   "SCAN:DONE"     // el esp32 avisa que termino de hacer el scaner
#define BT_COMMAND_RES_SCANN_DEVICE "SCAN:DEVICE"   // el es32 esta enviando la informacion de u ndispositivo, resultado del scan
#define BT_COMMAND_RES_SCANN_UPDATE "SCAN:UPDATE"