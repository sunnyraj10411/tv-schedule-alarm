// Main function and first file of tc

#include"Channel_crawler.h"

#define FILENAME_LEN 1024
#define DB_NAME							"./tvshow.db"
#define DB_CREATION_COMMAND	"sqlite3 tvshow.db < tvshow-db.sql"
using namespace std;


const string url_prefix = "http://tv.burrp.com";
int _g_db_ref_count = 0;
sqlite3 *_g_handle = NULL;

Channel_crawler::Channel_crawler(string name):channel_name(name)
{
	month_name_numeral["Jan"]="01";
	month_name_numeral["Feb"]="02";
	month_name_numeral["Mar"]="03";
	month_name_numeral["Apr"]="04";
	month_name_numeral["May"]="05";
	month_name_numeral["Jun"]="06";
	month_name_numeral["Jul"]="07";
	month_name_numeral["Aug"]="08";
	month_name_numeral["Sep"]="09";
	month_name_numeral["Oct"]="10";
	month_name_numeral["Nov"]="11";
	month_name_numeral["Dec"]="12";
}

static inline string &rtrim(string &s) {
        s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
        return s;
}

int db_close(sqlite3 *p_db_handle)
{


	int ret = -1;

	--_g_db_ref_count;

	if (_g_db_ref_count > 0) {
		 printf("remain DB ref_count(%d)\n", _g_db_ref_count);
		return 1;
	}

	if (NULL == p_db_handle) {
		 printf("DB was not opened\n");
		return 1;
	}

	ret = sqlite3_close(p_db_handle);

	if (SQLITE_OK != ret) {
		 printf("DB close failed. Error no(%d): %s\n", ret, sqlite3_errmsg(p_db_handle));
		return -1;
	}

	p_db_handle = NULL;

	return 1;
}

int db_open(sqlite3 **p_db_handle)
{


	int ret = -1;
	sqlite3 *handle = NULL;
	++_g_db_ref_count;

	if (_g_db_ref_count > 1) {
		 printf("already opened - DB ref_count(%d)\n", _g_db_ref_count);
		*p_db_handle = _g_handle;
		return 1;
	}

	ret = sqlite3_open(DB_NAME, &handle);

	if (SQLITE_OK != ret) {
		 printf(" DB open failed. Error no(%d): %s\n", ret, sqlite3_errmsg(handle));
		*p_db_handle = NULL;
		return -1;
	}

	*p_db_handle = handle;
	_g_handle = handle;

	 printf(" DB opened successfully, db = %p\n", handle);

	return 1;
}

int get_wday(const char *date_string)
{

	int day = (date_string[0] - 48) * 10 + (date_string[1] - 48);
	int month = (date_string[2] - 48) * 10 + (date_string[3] - 48);
	int year = (date_string[4] - 48) * 1000 + (date_string[5] - 48) * 100 + (date_string[6] - 48) * 10 + (date_string[7] - 48);

	struct tm date;
	time_t t = time(NULL);
	localtime_r(&t, &date);
	memset(&date, 0, sizeof(struct tm));

	date.tm_year = year - 1900;
	date.tm_mon = month - 1;
	date.tm_mday = day;

	time_t when = mktime(&date);
	const struct tm *norm = localtime(&when);

	return norm->tm_wday;
}

bool insert_channel_info_into_db(const char *channel_name, const char *title, const char *date, int wday, const char *time_string)
{

	int ret = -1;
	char *string_id = NULL;
	sqlite3_stmt *p_stmt = NULL;
	char *query = NULL;
	sqlite3 *p_db_handle = _g_handle;
	if ((NULL == p_db_handle)) {
		printf("Required parameters are NULL, DB Handle = %p\n", p_db_handle);
		return -1;
	}

	query = sqlite3_mprintf("INSERT INTO show_table (channel_name, title, date, wday, time, thumb, fav) VALUES ('%q', '%q', '%q', '%d', '%q', '%q', '%d');", channel_name, title, date, wday, time_string, NULL, 0);
	printf("Query is: %s\n", query);
	if (NULL == query) {
		printf("Unable to prepare the query\n");
		return false;
	}

	ret = sqlite3_prepare_v2(p_db_handle, query, strlen(query), &p_stmt, NULL);
	if (SQLITE_OK != ret) {
		printf("Query prepare failed. Error msg(%d): %s\n", ret, sqlite3_errmsg(p_db_handle));
		sqlite3_free(query);
		return false;
	}
	printf("Inserting statment Prepared successfully \n");

	ret = sqlite3_step(p_stmt);
	printf("step ret: (%d)\n", ret);

	sqlite3_finalize(p_stmt);
	p_stmt = NULL;
	sqlite3_free(query);

	printf("set ret: (%d), %s \n", ret, sqlite3_errmsg(p_db_handle));
	if (SQLITE_DONE == ret) {
		 printf("success, Leaving\n");
		return true;
	} else {
		 printf("Failed, Leaving>> ret: (%d), %s \n", ret, sqlite3_errmsg(p_db_handle));
		return false;
	}
}

bool Channel_crawler::extract_show_details(string show_string , ShowDetails &showDetails)
{
	regex e3("class=\"title\".*title=");	
	smatch sm3;
	regex_search(show_string,sm3,e3);
	//cout<<"Url show: "<<sm3[0]<<endl;	
	string processing_string = sm3[0];

	string search_pattern = "href=";
	size_t found = processing_string.find(search_pattern);	
	search_pattern = "title=";
	size_t found1 = processing_string.find(search_pattern);

	processing_string = processing_string.substr(found+6 , found1 - found - 8);
	
	processing_string = url_prefix + processing_string;		
	cout<<processing_string<<endl;

	string store;
	download_page(processing_string , "showdetailFile");
	read_file("showdetailFile",store);

	

	regex e5("Show Type.*showPics");
	smatch s5;
	regex_search(store , s5, e5);
	processing_string = s5[0];			
	cout<<processing_string<<endl;
	
	//extract_show_details_type_language_genre(processing_string , showDetails.Show_Type , showDetails.Language ,showDetails.Genre);	
	
}

bool Channel_crawler::create_json_file()
{
	map<string,Channel_json_file>::iterator itr = m_date_show_jsop_file.begin();
	cout<<"Sunny no of files: "<<m_date_show_jsop_file.size()<<endl;

	for(;itr != m_date_show_jsop_file.end(); itr++)
	{
		string file_name = "./output/" +channel_name+itr->first;
		ofstream json_file(file_name.c_str());
		if(json_file.is_open())
		{
			json_file<<itr->second<<endl;
		}
		
		cout<<itr->second<<endl;
		//break; //sunny remove break from here
			
	}
	
}

bool Channel_crawler::insert_into_db()
{
	map<string,Channel_json_file>::iterator itr = m_date_show_jsop_file.begin();
	for(;itr != m_date_show_jsop_file.end(); itr++)
	{
		Channel_json_file channel_json_file = itr->second;
		vector<Show> &showList = channel_json_file.listOfShows;

		for(vector<Show>::const_iterator itr1 = showList.begin(); itr1 != showList.end();itr1++)
		{
			insert_channel_info_into_db(channel_json_file.channelName.c_str(), itr1->showTitle.c_str(), channel_json_file.date.c_str(), get_wday(channel_json_file.date.c_str()), itr1->showTime.c_str());
		}
	}
}

bool Channel_crawler::extract_show(string show_string , Show &show)
{
	extract_show_title(show_string,show.showTitle);
	cout<<show.showTitle<<endl;
	extract_show_time(show_string,show.showTime);
	cout<<show.showTime<<endl;
	extract_show_thumb(show_string,show.showThumb);
	cout<<show.showThumb<<endl;
	//extract_show_details(show_string , show.showDetails);	 //sunny need to check what parameters we need to extract for show details. 
}

bool Channel_crawler::extract_show_thumb(string show_string , string &showThumb)
{
	regex e2("/images.*jpg");
	smatch sm2;
	regex_search(show_string, sm2,e2);
	//cout<<"Thumb nail: "<<sm2[0]<<endl;
	showThumb = sm2[0];
	showThumb = url_prefix + showThumb;
}

bool Channel_crawler::extract_show_title(string show_string , string &showTitle)
{
	regex e1("strong.*strong");
	smatch sm1;
	regex_search(show_string , sm1 ,e1);
	string processing_string = sm1[0];
	processing_string.erase(remove(processing_string.begin(), processing_string.end(), '\t'), processing_string.end());
 
	string search_pattern = ">";
	size_t found = processing_string.find(search_pattern);	
	search_pattern = "<";
	size_t found1 = processing_string.find(search_pattern);

	processing_string = processing_string.substr(found+1 , found1 - found - 1);
//cout<<"Show Title: "<<processing_string<<endl;	
	rtrim(processing_string);		

	showTitle = processing_string;
}

bool Channel_crawler::extract_show_time(string show_string , string &showTime)
{
	string processing_string;
	//Get time of the show

	regex e("from.*/sup>");
	smatch sm;
	regex_search(show_string , sm , e);
	//cout<<"Time: "<<sm[0]<<endl;

	processing_string = sm[0];
	processing_string.erase(remove(processing_string.begin(), processing_string.end(), '\t'), processing_string.end());
        processing_string.erase(remove(processing_string.begin(), processing_string.end(), ' '), processing_string.end());	

	//cout<<"Time: "<<processing_string<<endl;

	string search_pattern = ">";
	size_t found = processing_string.find(search_pattern);	
	search_pattern = "<";
	size_t found1 = processing_string.find(search_pattern);

	string time_ampm = processing_string.substr(found+1 , found1 - found - 1);
	//cout<<time_ampm<<endl;

	search_pattern = ">";
	size_t found3 = processing_string.find(search_pattern,found1+1);

	search_pattern = "<";
	size_t found4 = processing_string.find(search_pattern,found3+1);

	string ampm = processing_string.substr(found3+1 , found4 - found3 -1);
	//cout<<ampm<<endl;
	
	if( 1 ) // redundant if : did not want to do indentation 
	{
		//cout<<"sunny\n";
		stringstream time_add(time_ampm);
		int hour;
		char c;
		int minute;
		time_add>>hour;
		time_add>>c;
		time_add>>minute;

		if(ampm == "PM" && hour < 12)
		{	
			hour=hour+12;
		}
		
		//cout<<hour<<":"<<minute<<endl;
		stringstream convert;
		if(minute < 10 && hour < 10)
		{
			convert<<"0"<<hour<<":0"<<minute;
		}
		else if(minute < 10)
		{
			convert<<hour<<":0"<<minute;
		}
		else if(hour < 10)
		{
			convert<<"0"<<hour<<":"<<minute;
		}
		else
		{
			convert<<hour<<":"<<minute;
		}
	
		time_ampm = convert.str();
	}
	//cout<<time_ampm<<endl;	
	
	showTime = time_ampm+":00";	

	//SHOW title will be fetched now

}

bool Channel_crawler::extract_listofshows(string store_web_page , vector<Show> &listOfShows)
{
	regex e("dateHdr.*dateContainer");
	smatch sm;
	regex_search(store_web_page,sm,e);
	//cout<<sm[0]<<endl;

	string processing_string = sm[0];
	regex e1("<td class=\"resultTime\">.*</td>.*</tr>");
	smatch sm1;
	regex_search(processing_string , sm1 , e1);
	//cout<<sm1[0]<<endl;

	processing_string = sm1[0];
	//processing_string.erase(remove(processing_string.begin(), processing_string.end(), '\t'), processing_string.end());
        //processing_string.erase(remove(processing_string.begin(), processing_string.end(), ' '), processing_string.end());
		
	string end_pattern = "</tr>";
	vector<size_t> v_found;
	
	size_t found = processing_string.find(end_pattern);
	while(found != string::npos)
	{
		v_found.push_back(found);
		found = processing_string.find(end_pattern,found+1);
	}
	

	vector<string> v_show_string;
	size_t initial_pos = 0;

	for(vector<size_t>::iterator itr = v_found.begin() ; itr != v_found.end() ; itr++)
	{
		string show_string = processing_string.substr(initial_pos , *itr+5 - initial_pos);
		v_show_string.push_back(show_string);
		initial_pos = *itr+5;
	
		/*
		cout<<"-------------------\n";
		cout<<show_string<<endl;
		cout<<"-------------------\n";*/

		Show show;
		extract_show(show_string, show);
		listOfShows.push_back(show);
		//break; //sunny	
	}
}



bool Channel_crawler::get_current_date(string &current_date)
{
	time_t t = time(0);
	struct tm * now = localtime(&t);	
	//cout<<(now->tm_year + 1900) << ' '<< (now->tm_mon+1) << '-'<<now->tm_mday<<endl;

	stringstream day;
	stringstream month;
	stringstream year;

	year<<(now->tm_year + 1900);

	if(now->tm_mon+1 < 10)
	{
		month<<'0'<<now->tm_mon;
	}
	else
	{
		month<<now->tm_mon;
	}

	if(now->tm_mday < 10)
	{
		day<<'0'<<now->tm_mday;
	}
	else
	{
		day<<now->tm_mday;
	}

	current_date = day.str()+month.str()+year.str();	
}

void replace_string(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

bool Channel_crawler::download_extract_showtimings_details()
{
	//we will not print page of current date as it will be inconsistent as time keeps on changing.

	string current_date;
	get_current_date(current_date);

	cout<<current_date<<endl;	
 
	for(vector<pair<string,string> >::iterator itr = v_date_url.begin(); itr != v_date_url.end();itr++)
	{
		Channel_json_file channel_json_file;
		if(current_date != itr->first )
		{
	
			channel_json_file.date = itr->first;
			channel_json_file.channelName = channel_name;
			cout<<"date: "<<itr->first<<" url: "<<itr->second<<endl;
			string date_url;
			string store_web_page;
			date_url = itr->second;
			//cout<<date_url<<endl;
			replace_string(date_url, " ", "%20");
			//cout<<date_url<<endl;
			download_page(date_url,itr->first);
			read_file(itr->first,store_web_page);
			//cout<<store_web_page;
			extract_listofshows(store_web_page , channel_json_file.listOfShows);
			m_date_show_jsop_file[itr->first] = channel_json_file;
			//break; //sunny	
		}
	}		
}

bool Channel_crawler::extract_date_url_from_string(string url_day_date)
{
	regex e("a href=\".*\"");
	smatch sm;
	regex_search(url_day_date , sm , e);
	//cout<<sm[0]<<endl;

	string contains_url = sm[0];
	regex e1("\".*\"");
	smatch sm1;
	regex_search(contains_url,sm1,e1);
	//cout<<sm1[0]<<endl;

	string url = sm1[0];
	url.erase(url.begin());
	url.erase(url.end()-1);
	//cout<<url<<endl;

	regex e2("2015.*\"");
	smatch sm2;
	regex_search(url_day_date , sm2 , e2);
	//cout<<sm2[0]<<endl;
	string date_text = sm2[0];

	date_text.erase(remove(date_text.begin(), date_text.end(), '\t'), date_text.end());
	date_text.erase(remove(date_text.begin(), date_text.end(), ' '), date_text.end());

	//cout<<date_text<<endl;
	string month = date_text.substr(5,2);
	string date = date_text.substr(8,2);

	//cout<<"Month: "<<month<<" date: "<<date<<endl;
	string date_month_year = date+month+"2015";	 //**WARNING** have taken year 2015 by default need to change this for any kind of scalibilty
	//cout<<date_month_year<<endl;

	pair<string,string> temp_pair(date_month_year,url);
	v_date_url.push_back(temp_pair);
	
	return true;
}

bool Channel_crawler::date_url_finder(string base_url_page)
{
	regex e("dateIndicator.*dateHdr");
	//cout<<base_url_page<<endl;
	//string base_url_page1 = "dateIndicatordo sudo sudo dateIndicator dateHdr";
	//regex e("dateIndicator");	

	smatch cm;
	regex_search(base_url_page,cm,e);
	cout<<"string literal with: "<<cm.size()<<"matched\n";
	//cout<<cm[0]<<endl;

	smatch sm1;
	string search_string = cm[0];
	regex div("dateIndicator.*</div>.*</div>");
	regex_search(search_string,sm1,div);
	//cout<<sm1[0]<<endl;

	search_string = sm1[0];
	regex ahref("<a.*a>");
	regex_search(search_string , sm1 , ahref );
	//cout<<sm1[0]<<endl;

	search_string = sm1[0];	
	string end_pattern = "</a>";
	vector<size_t> v_found;


	size_t found = search_string.find(end_pattern);
	while(found != string::npos)	
	{
		v_found.push_back(found);
		//cout<<"Found at: "<<found<<endl;
		found = search_string.find(end_pattern,found+1);	
	}
	

	vector<string> v_url_day_date;
	size_t initial_pos = 0;;
	for(vector<size_t>::iterator itr = v_found.begin() ; itr != v_found.end() ; itr++)
	{
		//cout<<"in: "<<initial_pos<<" end: "<<*itr<<endl;
		string url_day_date = search_string.substr(initial_pos , *itr+4 - initial_pos);
		v_url_day_date.push_back(url_day_date);
		initial_pos = *itr+4;
		
		//cout<<"_____________________________\n";
		//cout<<url_day_date<<endl;
		//cout<<"_____________________________\n";
	}

	if(v_url_day_date.size())
	{
		for(vector<string>::iterator itr = v_url_day_date.begin() ; itr != v_url_day_date.end() ; itr++)
		{
			extract_date_url_from_string(*itr);
		}
	}
	else
	{
		return false;
	}
	return true;
		
}


//Function out of class called by libcurl library
size_t write_data(void *ptr,size_t size , size_t nmemb , FILE *stream)
{
	size_t written;
	written = fwrite(ptr,size,nmemb,stream);
	//cout<<(char *)ptr<<'\n';
	return written;
}

bool Channel_crawler::download_page(string url,string file_name)
{
	CURL *curl;
	FILE *fp;
	CURLcode res;
	const char *c_url = url.c_str();
	cout<<"Url name is: "<<c_url<<" \n";
	const char *outfilename = file_name.c_str();


	curl_global_init(CURL_GLOBAL_ALL);	
	curl = curl_easy_init();

	if(curl)
	{
		fp = fopen(outfilename,"w");
		curl_easy_setopt(curl,CURLOPT_URL,c_url);
		curl_easy_setopt(curl,CURLOPT_NOPROGRESS,1L);
		curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data);
		curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
		curl_easy_setopt(curl,CURLOPT_WRITEDATA , fp);
	 	res = curl_easy_perform(curl);	
		curl_easy_cleanup(curl);
		fclose(fp);
		return true;
	}
	else
	{
		return false;
	}
			
}
	
bool Channel_crawler::read_file(string filename , string &store)
{
	string line;
	ifstream myfile(filename.c_str());
	
	if(myfile.is_open())
	{
		while(getline(myfile,line))
		{
			store = store + line;
		}
		myfile.close();
		replace(store.begin(),store.end(),'\r',' ');
		//cout<<store;
		return true;
	}
	else
	{
		cout<<"Unable to openfile\n";
		return false;
	}
}


ostream & operator<<(ostream &output , Channel_json_file &channel_jason_file)
{
	output << "{\n"<<" \"date\": \""<<channel_jason_file.date<<"\",\n"
		<<" \"channelName\": \""<<channel_jason_file.channelName<<"\",\n"
		<<" \"listOfShows\": [\n"<<channel_jason_file.listOfShows<<" ]\n"<<"}\n";	      return output;	
}

ostream & operator<<(ostream &output , const vector<Show> &listOfShows)
{

	

	for(vector<Show>::const_iterator itr = listOfShows.begin(); itr != listOfShows.end();itr++)
	{
		output<<"  {\n"<<"   \"showTitle\": \""<<itr->showTitle<<"\",\n"
		               <<"   \"showTime\": \""<<itr->showTime<<"\",\n"
			       <<"   \"showThumb\": \""<<itr->showThumb<<"\",\n"
			       <<"   \"showDetails\": {}\n";

		vector<Show>::const_iterator tempitr = itr + 1;
		if(tempitr != listOfShows.end())
		{
		      output<<"  },\n";
		}
		else
		{
		      output<<"  }\n";
		}
		//break;	
	}
	return output;
}

int read_conf(string filename,vector<pair<string,string> > &v_channel_name_url)
{
        string line;
        ifstream myfile(filename.c_str());

        if(myfile.is_open())
        {
		cout<<"Reading configuration: \n";
                while(getline(myfile,line))
                {
        		cout<<line<<endl;
			regex e("channel>.*</channel");
			smatch sm;
			regex_search(line,sm,e);
		
			string processing_string = sm[0];	
			string search_pattern = ">";
			size_t found = processing_string.find(search_pattern);
		        search_pattern = "<";
        		size_t found1 = processing_string.find(search_pattern);

			processing_string = processing_string.substr(found+1 , found1 - found - 1);
			cout<<processing_string<<endl;



			regex e1("url>.*</url");
			smatch sm1;
			regex_search(line,sm1,e1);
			string processing_string1 = sm1[0];	
			search_pattern = ">";
			found = processing_string1.find(search_pattern);
		        search_pattern = "<";
        		found1 = processing_string1.find(search_pattern);

			processing_string1 = processing_string1.substr(found+1 , found1 - found - 1);
			cout<<processing_string1<<endl;

			pair<string,string> channel_name_url(processing_string,processing_string1); 		
			v_channel_name_url.push_back(channel_name_url);			

                }
                myfile.close();
                //replace(store.begin(),store.end(),'\r',' ');
                //cout<<store;
                return true;
        }
        else
        {
                cout<<"Unable to read_conf\n";
                exit(0);
        }

}

int main()
{
	int err = remove(DB_NAME);
	if( err == 0 ) {
		cout<<"db deleted successfully"<<endl;
	} else {
		cout<<"Unable to delete the file"<<endl;
		perror("Error");
	}

	system(DB_CREATION_COMMAND);
	sqlite3 *p_db_handle = NULL;
	db_open(&p_db_handle);

	vector< pair<string,string> > channel_name_url;
	read_conf("tv_crawler.conf",channel_name_url);

	for( vector<pair<string,string> >::iterator itr = channel_name_url.begin() ; itr != channel_name_url.end() ; itr++)
	{
		
		//string channel_name = "hbo";
		string channel_name = itr->first;
		Channel_crawler channel_crawler(channel_name);
		//string url = "http://tv.burrp.com/channel/hbo/8/";
		string url = "http://tv.burrp.com" + itr->second;
		string file = "./test";
		if( !channel_crawler.download_page(url,file))
		{
			cout<<"Error downloading page\n";
		}

		//S1 stage file downloaded find date and corrosponding url from this
		string base_url_page;
		if( !channel_crawler.read_file(file , base_url_page))
		{
			cout<<"Error reading downloaded file\n";
		}
		//cout<<base_url_page<<endl;
		if( !channel_crawler.date_url_finder(base_url_page))
		{
			cout<<"Error finding date and url\n";
		}

		channel_crawler.download_extract_showtimings_details();
		channel_crawler.insert_into_db();
//		channel_crawler.create_json_file();
	}
	db_close(p_db_handle);
}
