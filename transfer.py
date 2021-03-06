# Copyright (c) 2013, 2014 Gareth Scott
# transfer.py Copyright (c) Gareth Scott 2013 2014
# Tested with Python 2.6.4

# See: http://pyserial.sourceforge.net/pyserial_api.html#module-serial
import serial
# See: http://www.dabeaz.com/ply/ply.html
import lex
import yacc
import datetime as dt
from time import sleep
import time

# Globals
currentToken        = 0
OK                  = 0
ER                  = 1
# 4 = COM5 on Toshiba laptop. Look under Device Manager to determine com port
# 2 = COM3 on
# 21 = COM22 (Virtual COM port over USB often comes up as this value on Windows 7)
# /dev/ttyACM0 or similar on LinuxMint
# /dev/USB1234567 or similar on Mac
currentSerialPort   = 21   # May vary if using USB to COM port hardware

# List of token names.   This is always required
tokens = (
    'TOKEN_START',
    'TOKEN_END',
    'OK',
    'ER',
)

# Regular expression rules for simple tokens
t_TOKEN_START       = r'<'
t_TOKEN_END         = r'>'
t_OK                = r'OK'
t_ER                = r'ER'

def t_NUMBER(t):
    r'\d+'
    try:
        t.value = int(t.value)
    except ValueError:
        print "Integer too large:", t.value
        t.value = 0
    return t

# Error handling rule
def t_error(t):
    #print "Illegal character:'%s'" % t.value[0]
    t.lexer.skip(1)

lex.lex()

def p_OK(t):
    'expression : TOKEN_START OK TOKEN_END'
    global currentToken
    currentToken = OK

def p_ER(t):
    'expression : TOKEN_START ER TOKEN_END'
    global currentToken
    currentToken = ER

def p_error(t):
    print "Syntax error at:'%s'" % t.value
    while True:
        tok = yacc.token()             # Get next token
        if not tok or tok.type == 'TOKEN_START': break
    yacc.restart()

yacc.yacc()
    
class Transfer():
    def __init__(self, fileName):
        self.__fileName = fileName
        self.__lineCount = 1
        self.__haveSerialPort = True # Set to false for debugging with no serial port on computer
        if self.__haveSerialPort:
            self.__ser = serial.Serial(
                port     = currentSerialPort,
                baudrate = 115200,
                bytesize = serial.EIGHTBITS,
                parity   = serial.PARITY_NONE,
                stopbits = serial.STOPBITS_ONE,
                timeout  = 1,
                xonxoff  = 0,
                rtscts   = 0
            )
            print "Serial port:", self.__ser.isOpen()
            print self.__ser
        
        #self.cmdOK =    "<OK>"
        #self.cmdErr =   "<ERR>"
        self.EOT =      '$'     # End-of-transmission
        
        self.parseStr = ""

    def send(self):
        f = open(self.__fileName)
        for line in iter(f):
            if self.sendLine(line) == False:
                break
        f.close()
        self.__ser.close()
        
    def sendLine(self, line):
        line = line.rstrip('\r\n')
        print '{0}: {1}'.format(self.__lineCount, line)
        self.__lineCount = self.__lineCount + 1
        if self.__haveSerialPort:
            line = line + self.EOT 
            self.__ser.write(line)
            if self.parseInputTimeout() == False:
                return False
            # read input, allow delay
            # if <ERR> resend x2 then exit with fatal error
        return True

    def parseInputTimeout(self):
        global currentToken
        # 1. Get current time
        startTime = time.time()
        deltaTime = 0.0
        #self.parseInput()
        #return True
        currentToken = ER
        while deltaTime < 2.0: 
            # 2. Call parseInput
            self.parseInput()
            # 3. Got OK return True; got ER, return False
            if currentToken == OK:
                #print 'OK'
                return True
            sleep(0.002)
            deltaTime = time.time() - startTime
            # 4. If timer not expired, goto 2 above
        print 'ERROR'
        return False
        
    def parseInput(self):
        if self.__haveSerialPort:        
            waitingCount = self.__ser.inWaiting()
            #waitingCount = 1
            if waitingCount == 0:
                return
            x = self.__ser.read(waitingCount)
            self.parseStr += x
        else:
            self.parseStr = ' <OK> '
        while len(self.parseStr) >= 1:
            #print "before:", self.parseStr
            foundEndTokenIndex = self.parseStr.find(">")
            if foundEndTokenIndex != -1:
                # Found '>' token delimiter
                newTokenString = self.parseStr[:foundEndTokenIndex+1]
                foundBeginTokenIndex = newTokenString.rfind("<")
                if foundBeginTokenIndex != -1 and foundBeginTokenIndex < foundEndTokenIndex:
                    yaccInput = newTokenString[foundBeginTokenIndex:]
                    if yaccInput != '<OK>':
                        print "yacc:", yaccInput
                    currentToken = 0
                    yacc.parse(yaccInput, debug=1)
                # Remove up to end of token from string
                self.parseStr = self.parseStr[foundEndTokenIndex+1:]
                #print "after:", self.parseStr
            else:
                break
            
    def echoInput(self):
        if self.__haveSerialPort:        
            waitingCount = self.__ser.inWaiting()
            #waitingCount = 1
            if waitingCount == 0:
                return
            x = self.__ser.read(waitingCount)
            self.parseStr += x
        else:
            self.parseStr = ' <OK> '
        print self.parseStr,    # , = don't print CR after each string
        self.parseStr = ''
            
if __name__ == "__main__":
    downloadProgram = 1
    downloadRun = 1
    downloadStop = 0
    downloadContinue = 0
    downloadFlash = 0
    monitorComPort = 0

    if (downloadProgram):
        startTime = time.time()
        t = Transfer("/dev/docs/scottdesign/parser/tree.txt")
        t.send()
    
        t = Transfer("/dev/docs/scottdesign/parser/symbolTable.txt")
        t.send()
    
        print 'Done in {0:.2f} sec'.format(time.time() - startTime)

    if (downloadRun):
        t = Transfer("/dev/docs/scottdesign/parser/prun.txt")
        t.send()
    
    if (downloadStop):
        #sleep(5);
        t = Transfer("/dev/docs/scottdesign/parser/stop.txt")
        t.send()

    if (downloadContinue):
        #sleep(5);
        t = Transfer("/dev/docs/scottdesign/parser/continue.txt")
        t.send()

    if (downloadFlash):
        #sleep(5);
        t = Transfer("/dev/docs/scottdesign/parser/flash.txt")
        t.send()

    if (monitorComPort):
        # Hangs virtual COM port. Need to unplug cable after program exits to reset.
        t = Transfer("Dummy name just to get serial port opened")
        while 1:
            t.echoInput()
    
