

#Linux terminal

The LOSS_RATE ERR_RATE is simulation of packet loss&&pack corrupt during the transmission 

Aiming to test the reliability of the protocal

##Run server:

    sh run-server.sh LOSS_RATE ERR_RATE clientIp

##Run client:

    sh run-client.sh LOSS_RATE ERR_RATE serverIp filename

#Example

##Run server:

    sh run-server.sh 0.1 0.1 localhost

##Run client:

    sh run-client.sh 0.1 0.1 localhost
