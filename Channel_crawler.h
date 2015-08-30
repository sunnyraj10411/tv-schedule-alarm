// Main function and first file of tc

#include<iostream>
#include<fstream>
#include<map>
#include<vector>
#include<string>
#include<algorithm>
#include<regex>
#include<curl/curl.h>
#include<curl/easy.h>
#include<stdio.h>
#include<regex>
#include<ctime>
#include<cstring>
#include<sqlite3.h>

using namespace std;

int read_conf(string filename , vector<pair<string,string> > &channel_name_url);

struct ShowDetails{
	string Language;
	string Repeats_on;
	string Cast;
	string Genre;
	string Preceded_By;
	string Show_Type;
	string Show_Description;
};

struct Show{
	string showTitle;
	string showTime;
	string showThumb;
	ShowDetails showDetails;
};

struct Channel_json_file{
	string date;	
	string channelName;
	vector<Show> listOfShows;
};

class Channel_crawler{

	string channel_name;
	string channel_url;
	map<string,string> month_name_numeral;
	vector< pair<string,string> > v_date_url;
	map<string,Channel_json_file> m_date_show_jsop_file;

	public:
	Channel_crawler(string name);
	bool download_page(string url,string file_name);
	bool read_file(string filename , string &store);
	bool date_url_finder(string base_url_page);
	bool extract_date_url_from_string(string url_day_date);
	bool download_extract_showtimings_details();
	bool get_current_date(string &current_date);
	bool extract_listofshows(string store_web_page , vector<Show> &listOfShows);
	bool extract_show(string show_string , Show &show);
	bool extract_show_time(string show_string , string &showTime);
	bool extract_show_title(string show_string , string &showTitle);
	bool extract_show_thumb(string show_string , string &showThumb);
	bool extract_show_details(string show_string , ShowDetails &showDetails);
	bool create_json_file();
	bool insert_into_db();
};

ostream & operator<<(ostream &output , Channel_json_file &channel_jason_file);
ostream & operator<<(ostream &output , const vector<Show> &listOfShows);
		






