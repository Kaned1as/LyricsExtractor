#include "config.h"
#include <iostream>
#include <mysql.h>
#include <id3/tag.h>

using namespace std;
MYSQL *DB_connect()
{
    MYSQL *conn = mysql_init(NULL);
    char hostname[32], user[16], pass[16], dbname[16];
    cout << "DB hostname [localhost]" << endl;
    gets(hostname);
    if (!strlen(hostname));
        strcpy(hostname, "localhost");
    cout << "DB user [amarokuser]" << endl;
    gets(user);
    if (!strlen(user));
        strcpy(user, "amarokuser");
    cout << "DB password" << endl;
    gets(pass);
    cout << "DB name [amarokdb]" << endl;
    gets(dbname);
    if (!strlen(dbname));
        strcpy(dbname, "amarokdb");

    if (!mysql_real_connect(conn, hostname, user, pass, dbname, 0, NULL, 0))
    {
        cout << mysql_error(conn);
        return NULL;
    }
    return conn;
}

int main()
{
    // connect to Amarok MySQL DB
    MYSQL *connection = DB_connect();
    MYSQL_RES *result;
    MYSQL_ROW row;
    string file_path, file_name, lyrics_content;
    if (connection)
        cout << "Connection succesful" << endl;
    else
        return -1;

    // They made me do this <.<
    mysql_query(connection, "SET NAMES utf8");
    // Getting lyrics info...
    if (mysql_query(connection, "SELECT `url`, `lyrics` FROM `lyrics`"))
    {
        cout << mysql_error(connection);
        return -1;
    }

    // Do we have one?
    result = mysql_use_result(connection);
    while ((row = mysql_fetch_row(result)) != NULL)
    {
        // Some useful vars
        file_path = row[0];
        lyrics_content= row[1];

        file_path = file_path.substr(1);
        file_name = file_path.substr(file_path.find_last_of("/")+1);
        ifstream curr_file(file_path.c_str());
        // Getting linked file...
        if(curr_file)
        {
            curr_file.close();
            // Let's analyze it's ID3 Tag
            ID3_Tag ScanTag(file_path.c_str());
            // Ensure that the one doesn't have lyrics already...
            if (!ScanTag.Find(ID3FID_UNSYNCEDLYRICS))
            {
                cout << "processing file " << file_name << " - ";
                ID3_Frame *lyrics_frame = new ID3_Frame(ID3FID_UNSYNCEDLYRICS);

                // ... and is ready for insertion
                if(!ScanTag.AttachFrame(lyrics_frame))
                {
                    cout << "Error: can't attach lyrics frame to tag" << endl;
                    delete lyrics_frame;
                    continue;
                }

                // KEY LINE!!!
                // Making file's ID3 Tag use lyrics from the Amarok DB
                lyrics_frame->GetField(ID3FN_TEXT)->Set(lyrics_content.c_str());
                // Write it in!
                ScanTag.Update();
                cout << "Successful update" << endl;
            }
            else
                cout << file_name << " already has lyrics" << endl; // so cute...
        }
        else
            cout << "file " << file_name << " doesn't exist" << endl;
    }

    // All done, exiting
    mysql_close(connection);
    return 0;
}
