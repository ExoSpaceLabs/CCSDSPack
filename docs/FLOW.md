# FLOW

[../](README.md) - CCSDSPack Documentation

the data flow is shown here...

Lets assume we get a bufffer that contains a CCSDS packet call it "buff".

Possibly with the optional setting of error check and ordered check

After the CCSDSPack has been defined:
* Load(buffer) or Load("./file.bin")
* Check minimum size CCSDS packet PRIMARY HEADER + ERROR CHECK throw error if needed.
* Analyse if it by getting the data for HEADER and take out DATA LENGTH throw error if data after is not header
* check if another header is after the ERROR CHECK.
* Populate member variable vector if constraints are met.
  * if multiple packets are loaded check for sequence flag continue and end.
  * check for count in incrememntal order.
