<title>SigGen Component Dictionary</title>
# SigGen Component Dictionary


## Command List

|Mnemonic|ID|Description|Arg Name|Arg Type|Comment
|---|---|---|---|---|---|
|SIGGEN_SetChirp|0 (0x0)|| | |   
| | | |omega_i|F64||                    
| | | |omega_f|F64||                    
| | | |amplitude|F64||                    
| | | |duration|F64||                    
| | | |offset|F64||                    
|SIGGEN_SetAxis|1 (0x1)|| | |   
| | | |x|F64||                    
| | | |y|F64||                    
| | | |z|F64||                    
|SIGGEN_DoChirp|2 (0x2)|| | |   
| | | |mode|ChirpMode||                    
| | | |index|U8||                    
|SIGGEN_Cancel|3 (0x3)|| | |   

## Telemetry Channel List

|Channel Name|ID|Type|Description|
|---|---|---|---|
|SIGGEN_ThrustComm|0 (0x0)|F32|dummy|

## Event List

|Event Name|ID|Description|Arg Name|Arg Type|Arg Size|Description
|---|---|---|---|---|---|---|
|SIGGEN_Dummy|0 (0x0)|dummy event| | | | |