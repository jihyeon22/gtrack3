<<<<<<< HEAD

MDT800_SRC = ext/protocol/mdt800/file_mileage.o \
             ext/protocol/mdt800/geofence.o \
             ext/protocol/mdt800/gps_filter.o \
             ext/protocol/mdt800/gps_utill.o \
             ext/protocol/mdt800/gpsmng.o \
             ext/protocol/mdt800/hdlc_async.o \
             ext/protocol/mdt800/packet.o \
             ext/protocol/mdt800/sms.o \
       
OBJS_MODEL += $(MDT800_SRC)
CFLAGS += -I./ext/protocol/
=======

MDT800_SRC = ext/protocol/mdt800/file_mileage.o \
             ext/protocol/mdt800/geofence.o \
             ext/protocol/mdt800/gps_filter.o \
             ext/protocol/mdt800/gps_utill.o \
             ext/protocol/mdt800/gpsmng.o \
             ext/protocol/mdt800/hdlc_async.o \
             ext/protocol/mdt800/packet.o \
             ext/protocol/mdt800/sms.o \
       
OBJS_MODEL += $(MDT800_SRC)
CFLAGS += -I./ext/protocol/
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
