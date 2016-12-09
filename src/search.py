import sys
import mysql.connector

'''
Simple program for querying our search database

Given a keyword as an argument, pulls url's that contain the keyword.
'''

if len(sys.argv) != 2:
    print 'usage: python search.py <keyword>'
    sys.exit(1)

cnx = mysql.connector.connect(user='user', password='password',
        host='127.0.0.1', database='Search')

# Make sure that data is valid utf-8
valid = True
keyword = "'%" + sys.argv[1] + "%'"
try:
    keyword.decode('utf-8')
except UnicodeError:
    valid = False
    print 'Invalid input'

if (valid):

    # create cursor object
    cursor = cnx.cursor(buffered=True)

    # Execute search query
    query = "SELECT url FROM url_list WHERE url LIKE " + keyword
    query += " OR data LIKE " + keyword
    cursor.execute(query)

    # fetch all rows from the query
    data = cursor.fetchall()

    # print the rows
    for row in data:
        print row[0]

    # close the cursor object
    cursor.close()

# close the connection
cnx.close()
sys.exit()

