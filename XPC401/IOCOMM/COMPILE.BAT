COPY ..\HDR\COMMASM.H
MASM IOCOMM.ASM;
DEL COMMASM.H
MSC %1 /Gs IOUTIL.C;
