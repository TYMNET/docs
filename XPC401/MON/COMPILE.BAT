MSC %1 /Gs DATA.C;
MSC %1 /Gs DUMP.C;
COPY ..\HDR\COMMASM.H
MASM IOC2;
DEL COMMASM.H
MSC %1 /Gs IOU2.C;
MSC %1 /Gs MON.C;
MSC %1 /Gs MONCHMOD.C;
MSC %1 /Gs MONCLINK.C;
MSC %1 /Gs MONDATA.C;
MSC %1 /Gs MONERROR.C;
MSC %1 /Gs MONPLINK.C;
MSC %1 /Gs MONVPKT.C;
MSC %1 /Gs PKTMSG.C;
