REM There are two  715 LINT informational messages which will occur.  
REM These errors occur because routines are called from a jump table with
REM variables which they do not need.  The other functions called from the
REM jump table need these variables

LINT XPC APPL IOC LINK PKT UTIL > LINT.DAT


