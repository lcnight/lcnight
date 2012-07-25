import re
import conf

def htmlunicode2textgbk ( matchobj ):

    return unichr(int(matchobj.group(0)[2:-1])).encode('gbk') 
    
    
def fxparse ( c ):
    
    return re.sub( '&#(\d+);', htmlunicode2textgbk, c )


def langparse ( c ):
    
    c = c.splitlines()
    
    for i in range(len(c)) :
        if c[i].startswith('Language=') :
            c[i] = 'Language=0x804'
            break
            
    c = '\n'.join(c)
    
    return c

if __name__=="__main__":
    
    import sys
    
    filename = sys.argv[1] + '/' + conf.htmlhelp_basename + '.hhc'
    
    fp = open( filename , 'r' )
    
    c = fp.read()
    
    fp.close()
    
    c = fxparse(c)
    
    fp = open( filename , 'w' )
    
    fp.write(c)
    
    fp.close()
    
    print 'covert hhc in GBK.'
    
    filename = sys.argv[1] + '/' + conf.htmlhelp_basename + '.hhp'
    
    fp = open( filename , 'r' )
    
    c = fp.read()
    
    fp.close()
    
    c = langparse(c)
    
    fp = open( filename , 'w' )
    
    fp.write(c)
    
    fp.close()
    
    print 'fix the lang set in hhp.'
    
    