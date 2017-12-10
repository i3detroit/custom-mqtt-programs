#!/usr/bin/python
'''
Created on Dec 8, 2011

@author: Matt
'''
import serial
import time
import sys



def txPacket(packet, ser):
    print "Transmitting:"
    print packet
    for x in packet:
        #print ord(x)
        ser.write(x)

def createResetPacket():
    ''' Creates and returns a reset packet'''
    d = list()
    d.append(chr(3)) #Message ID
    d.append(chr(127)) #always 7F hex
    d.append(chr(7)) #number of chars in packet including MID and checksum
    d.append(chr(65)) #transmission ID (0x41 = reset)
    d.append(chr(1*16 + 0))   #number of packets in this transmission + this packet's position (0 indexed)
    #data begins here
    d.append(chr(0)) #always 00 hex
    appendChecksum(d)
    return d



def appendChecksum(d):
    '''Calculates #2's compliment of sum of all data in the passed packet parameter'''
    sum = 0
    for x in d:
        sum += ord(x)
    checksum = (~sum % 256)+1
    #print "checksum"
    #print checksum  #2's compliment of sum of all preceding data.
    d.append(chr(checksum))
    return d

def sendTransmission(s, xmitID, ser):
    #create a list of text chunks 14 chars long
    txtChunks = nsplit(s, 14)
    count = 0
    for i in txtChunks:
    #calculate how big the packet will need to be
        p = list()
        p.append(chr(3)) #Message ID
        p.append(chr(127)) #always 7F hex
        p.append(chr(7 + len(i))) #number of chars in packet including MID and checksum
        p.append(chr(xmitID)) #transmission ID
        p.append(chr((len(txtChunks))*16 + count))   #number of packets in this transmission + this packet's position
        #data begins here
        for char in i:
            p.append(char)
        p.append(chr(0)) #always 00 hex
        appendChecksum(p)
        #transmit packet
        #print "tx function created:"
        #print p
        txPacket(p, ser)
        count = count + 1

def printText(s, ser):
    '''Accepts a string and prints it to the screen'''
    # This largely doesn't work to split a program into multiple transmissions. There just isn't enough memory in the sign :(
    txmissions = nsplit(s, 140)
    #print "transmissions are:"
    #print txmissions
    print "Sending " + str(len(txmissions)) + " transmissions"
    xmitID = 0
    for i in txmissions:
        print "Sending Txmission: " + i
        sendTransmission(i, xmitID, ser)
        xmitID = xmitID + 1
        time.sleep(.2)


def nsplit(s, n):
    return [s[k:k+n] for k in xrange(0, len(s), n)]


def main():
    text = sys.argv[1];
    ser = serial.Serial('/dev/ttyUSB0',9600,timeout = 1,writeTimeout = 1)
    print ser

    #First create and send a reset packet
    #send bs to sign.
    txPacket(createResetPacket(), ser)

     # delay 100 ms
    time.sleep(.1)
    print text
    #printText("^L5^S" + text + "^F", ser)
    printText(text, ser)

    time.sleep(.5)

    print "Closing Port"
    ser.close()




if __name__ == '__main__':
    main()
