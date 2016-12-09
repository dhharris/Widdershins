'''
This program checks to see if a url is in the MySQL database
'''

import mysql.connector
import sys
    
# Make sure that encoding is set to utf8
reload(sys)  
sys.setdefaultencoding('utf8')


if len(sys.argv) != 2:
    print 'usage: python check.py <url>'
    sys.exit(1)

url = sys.argv[1]

cnx = mysql.connector.connect(user='user', password='password',
        host='127.0.0.1', database='Search')


cursor = cnx.cursor()
query = 'SELECT COUNT(*) FROM url_list WHERE url= "' + url + '"'
cursor.execute(query)
result = cursor.fetchone()

if result[0] > 0:
    exists = True
else:
    exists = False
    
with open('.check.txt', 'w+') as outfile:
    if exists:
        outfile.write('1')
    else:
        outfile.write('0')
    