#include "config.h"

#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <getopt.h>

#include <mysql.h>

#include <mpegfile.h>
#include <id3v2tag.h>
#include <tstring.h>
#include <unsynchronizedlyricsframe.h>

using namespace std;
typedef multimap<string, string> StringList;
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

bool fexist(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0 && (buffer.st_mode & (S_IFREG | S_IFLNK)))
        return true;
    return false;
}

void fill_file_array(vector<string> *filelist, string dirname)
{
    DIR *searchdir = opendir(dirname.c_str());
    dirent *item;
    if (searchdir)
    {
        while ((item = readdir(searchdir)) != NULL)
        {
            //puts(item->d_name);
            if (item->d_type == DT_REG)
            {
                filelist->push_back(dirname + "/" + string(item->d_name));
            }
            else
                if (item->d_type == DT_DIR && strcmp(item->d_name, ".") != 0 && strcmp(item->d_name, "..") != 0)
                    fill_file_array(filelist, string(dirname + "/" + string(item->d_name)).c_str());

        }
        closedir(searchdir);
    }
}

StringList sort_file_array(vector<string> filelist)
{
    StringList sflist;
    for(vector<string>::iterator itr = filelist.begin(); itr != filelist.end(); itr++)
    {
        string file_ext = itr->substr(itr->find_last_of(".")+1);
        // make *.MP3 *.mp3 if exists
        transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
        sflist.insert(pair<string, string>(file_ext, (*itr)));
    }
    return sflist;
}

void pure_search(const char *dirname)
{
    vector<string> filelist;
    StringList filelist_s;
    fill_file_array(&filelist, dirname);
    filelist_s = sort_file_array(filelist);

    for(StringList::iterator itr = filelist_s.lower_bound("mp3"); itr != filelist_s.upper_bound("mp3"); itr++)
    {
        cout << "key " << itr->first << " value " <<itr->second << endl;
    }
}

int main(int argc, char *argv[])
{
    bool overwrite = false;
    bool with_amarok = false; //for future use

    int opt;
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"with-amarok", no_argument,       0, 'A'},
            {"overwrite",   no_argument,       0, 'O'},
            {0, 0, 0, 0}
        };
        int opt_index = 0;
        opt = getopt_long(argc, argv, "AO", long_options, &opt_index);

        if (opt == -1)
            break;

        switch(opt)
        {
            case 'A':
                with_amarok = true;
                break;
            case 'O':
                overwrite = true;
                break;
        }
    }

    pure_search("/home/adonai/Музыка");
    return 0;

    // connect to Amarok MySQL DB
    MYSQL *connection = DB_connect();
    MYSQL_RES *result;
    MYSQL_ROW row;
    string file_path, file_name;
    TagLib::String lyrics_content;
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
        lyrics_content= TagLib::String(row[1], TagLib::String::UTF8);

        file_path = file_path.substr(1);
        file_name = file_path.substr(file_path.find_last_of("/")+1);
        // Getting linked file...
        if(fexist(file_path.c_str()))
        {
            // Let's analyze it's ID3 Tag
            TagLib::MPEG::File curr_file(file_path.c_str());
            if(!curr_file.ID3v2Tag())
                continue;

            TagLib::ID3v2::FrameList framelist = curr_file.ID3v2Tag()->frameListMap()["USLT"];
            // Ensure that the one doesn't have lyrics already...
            if (framelist.isEmpty())
            {
                cout << "processing file " << file_name << " - ";
                TagLib::ID3v2::UnsynchronizedLyricsFrame *lyrics_frame = new TagLib::ID3v2::UnsynchronizedLyricsFrame();

                // KEY LINE!!!
                // Making file's ID3 Tag use lyrics from the Amarok DB
                lyrics_frame->setTextEncoding(TagLib::String::UTF8);
                lyrics_frame->setText(lyrics_content);
                curr_file.ID3v2Tag()->addFrame(lyrics_frame);
                // Write it in!
                curr_file.save();
                cout << "Successful update" << endl;
            }
            else
                if (overwrite) // Delete existing lyrics and set ours...
                {
                    cout << "overwriting tag in file " << file_name << " - ";
                    TagLib::ID3v2::UnsynchronizedLyricsFrame *lyrics_frame = (TagLib::ID3v2::UnsynchronizedLyricsFrame*)framelist.front();

                    lyrics_frame->setTextEncoding(TagLib::String::UTF8);
                    lyrics_frame->setText(lyrics_content);
                    curr_file.save();
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
