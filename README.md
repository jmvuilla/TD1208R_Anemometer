# TD1208R_Anemometer
Source code of the firmware running in the Telecom Design TD1208R module. Main tasks:
- measure the windspeed with a period of 1mn and store the measurement in a 10-byte buffer,
- every 10mn, measure the TD1208R core voltage (2 bytes) and send the windspeed measurements + voltage over the Sigfox network.
