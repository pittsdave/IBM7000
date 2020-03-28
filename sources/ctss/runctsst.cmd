#
# CTSS Run command file
#
lt        Load Tape
# TRA 0100 - Transfer to CTSS kernel:
er 002000000100
# The following enables trace on A9:
#ek 525252000000
# Set the SYSNAM in the AC "CTSS  ":
ea 236362626060
# Turn on SSW to disable:
#sw1      FIBMON
sw2      DAEMON
#
st        STart
