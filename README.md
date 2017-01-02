# TD1208R_Anemometer
Source code of the firmware running in the Telecom Design TD1208R module. 
- Over a period of 10mn, measure the average wind speed during 1mn - resulting in 10 wind speed samples. Wind speed is stored as an 8-bit integer, in a 10-slot buffer.
- At the end of the 10mn period, measure the TD1208R core voltage (value stored in a 16-bit integer). The resulting wind speed and core voltage measurements (12 bytes in total) are sent over the Sigfox network.
