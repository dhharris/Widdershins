import mysql.connector
import sys
import os
import re
from HTMLParser import HTMLParser
from Carbon.Aliases import true

class MetaParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.data = ''
    
    def handle_starttag(self, tag, attr):
        if tag != 'meta':
            return
        desc = False
        for name, value in attr:
            if name == 'name' and value == 'description':
                desc = True
            elif desc and name == 'content':
                self.data = value
                return
                
    def handle_endtag(self, tag):
        return
    def handle_data(self, data):
        return

'''
This program inserts a url into the search.url_list table.
It requires the url to be inserted to be passed in as an argument
'''
    
# Make sure that encoding is set to utf8
reload(sys)  
sys.setdefaultencoding('utf8')


if len(sys.argv) != 3:
    print 'usage: python insert.py <url> <data>'
    sys.exit(1)

cnx = mysql.connector.connect(user='user', password='password',
        host='127.0.0.1', database='Search')

# Make sure that data is valid utf-8
valid = True

url = sys.argv[1]
raw_data = sys.argv[2]

# find the title of the page, if it exists
index = raw_data.find('<title>')
if index != -1:
    title = raw_data[index + len('<title>'):raw_data.find('</title>')]
    title = title[:100]
else:
    title = 'The title for this webpage could not be displayed.'



# Find where metadata starts, if at all
# This makes sure that the data we receive includes keywords
# That we can later use to search
index = raw_data.find('<meta')
if index != -1:
    raw_data = raw_data[index:]
else:
    # Start at <body>
    index = raw_data.find('<body')
    if index != -1:
        raw_data = raw_data[index:]
        

# Only first 2048 bytes of data
raw_data = raw_data[:2048]

try:
    raw_data.decode('utf-8').encode('ascii', 'replace')
except UnicodeError:
    valid = False
if (valid):
    # Try to parse description from metadata
    p = MetaParser()
    p.feed(raw_data)
    if p.data != '':
        description = p.data[:297]
        if len(p.data) > 300:
            description = description + '...'
    else:
        description = 'No description available'
    p.close()
    
    data = (title, description, url, raw_data)
    cursor = cnx.cursor()
    query = "INSERT INTO url_list (title, description, url, data) VALUES (%s, %s, %s, %s)"

    try:
        cursor.execute(query, data)
    except mysql.connector.errors.IntegrityError:
        pass


    # Make sure data is committed to the database
    cnx.commit()

    cursor.close()
cnx.close()
