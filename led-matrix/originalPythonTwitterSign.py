'''
Created on Dec 8, 2011

@author: Matt
'''
import twitter
import serial
import time
import itertools
import traceback


last_text = ''

def sanitizeText(text):
    ''' Strips any suspicious junk like ^'s and ~ for the sign'''
    text = text.replace('^', '')
    text = text.replace('~', '')
    #filter out html junk
    text = text.replace('&lt;','<')
    text = text.replace('&gt;','>')
    
    return text

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
    


def update(api):
    ''' fetches the most recent tweet and pushes to the sign'''
    global last_text

    #try:
        #status = api.GetUserTimeline(count = 1)
    status = api.GetMentions()
    dmsgs = api.GetDirectMessages()
    if (status[0].created_at_in_seconds < dmsgs[0].created_at_in_seconds):
        text = dmsgs[0].text
    else:
        text = status[0].text
    #print status[0]
    text = sanitizeText(text)
    #have to do this cause the memory of this sign is crap :/
    text = text[0:77]
    #except:
    #print "failed to get status update. Check internet connection or something... :("
    #text = last_text

    
    
    if (text != last_text):
        print "Found New Tweet: " + text
        last_text = text
        #ser = serial.Serial('/dev/tty.usbserial',9600,timeout = 1,writeTimeout = 1)
        #print ser

        #First create and send a reset packet
        #send bs to sign.
        #txPacket(createResetPacket(), ser)

         # delay 100 ms
        time.sleep(.1)
        print text
        #printText("^L5^S" + text + "^F", ser)

        time.sleep(.5)
        #print "Closing Port"
        #ser.close()


def main():
    #grab bs from twitter
    # get these at https://dev.twitter.com/apps/new
    #These work for matt oehrlein's twitter account
    #token='16338238-6EOoHLcNCClZkaiqdk70VsHAutRbU9oPchF2b5no'
    #token_secret='310gPvZzZexNhRj1HIKead59iQBgpzxaU5gEG4UlYU'
    #con_key='Yip0KeNRSJqsR6WAYMeKJQ'
    #con_secret='lXJGPUnirFBJiqyVD6UVbuXhOGtcIkfkc6OsLlfK0I'
    #these work for i3detroit's twitter account:
    con_key = 'aDg43i13Xp8R2WG0EmCDw'
    con_secret = '6YgULSzBCjONAissOBdM7v0s4QYzQYEqklpxhvk2ks'
    token = '34795587-4LVLCd35Ep8o14puhTV8uaugiUpJbmI0YGf4RbZTu'
    token_secret = 'eer3qc43XbKP5HgEQexfpXPLUMrXy9MUUjvE7wF7dY'
    
    api = twitter.Api(consumer_key = con_key,
                      consumer_secret = con_secret,
                      access_token_key = token,
                      access_token_secret = token_secret)
    #api.GetUser('MattOehrlein')
    myself = api.VerifyCredentials()
    #print myself
    while(True):
        update(api)
        time.sleep(28)
    


if __name__ == '__main__':
    main()