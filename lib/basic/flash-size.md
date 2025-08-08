
## snprintf

implem        | taille flash
--------------|---------------
sans snprintf | 446835
cat           | 446903 (+ nom 8.3)
error         | 446887
free          | 446887
bastos        | 446851
wifi list     | 446835

## websockets lib

-110 KB

## DB

implem        | flash  | RAM ESP
--------------|--------|---------
Initial       | 341239 | 10448
WiFi en DB    | 340239 | 10664/9582(after cx wifi) / 8696(after cx server)
save/load cfg | 340111 | 11432/9776(after cx wifi) / 9312(after cx server)
db set        | 340127 | -
Serveurs en DB| 340015 | 11512/9776(after cx wifi) / 9504(after cx server)/9264(plusieurs cx)
