#ifndef DATE_TIME_TZ_DB_BASE_HPP__
#define DATE_TIME_TZ_DB_BASE_HPP__

/* Copyright (c) 2003-2005 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/time_zone_names.hpp>
#include <boost/date_time/time_zone_base.hpp>
#include <boost/date_time/time_parsing.hpp>
#include <boost/algorithm/string.hpp>

namespace boost {
  namespace date_time {

    //! Exception thrown when tz database cannot locate requested data file
    class data_not_accessible : public std::logic_error
    {
     public:
       data_not_accessible() : 
         std::logic_error(std::string("Unable to locate or access the required datafile.")) 
       {}
       data_not_accessible(const std::string& filespec) : 
         std::logic_error(std::string("Unable to locate or access the required datafile. Filespec: " + filespec)) 
       {}
    };
    
    //! Exception thrown when tz database locates incorrect field structure in data file
    class bad_field_count : public std::out_of_range
    {
     public:
       bad_field_count(const std::string& s) : 
         std::out_of_range(s) 
      {}
    };

    //! Creates a database of time_zones from csv datafile
    /*! The csv file containing the zone_specs used by the
     * tz_db_base is intended to be customized by the
     * library user. When customizing this file (or creating your own) the
     * file must follow a specific format.
     * 
     * This first line is expected to contain column headings and is therefore
     * not processed by the tz_db_base.
     *
     * Each record (line) must have eleven fields. Some of those fields can
     * be empty. Every field (even empty ones) must be enclosed in 
     * double-quotes.
     * Ex:
     * @code
     * "America/Phoenix" <- string enclosed in quotes
     * ""                <- empty field
     * @endcode
     * 
     * Some fields represent a length of time. The format of these fields 
     * must be:
     * @code
     * "{+|-}hh:mm[:ss]" <- length-of-time format
     * @endcode
     * Where the plus or minus is mandatory and the seconds are optional.
     * 
     * Since some time zones do not use daylight savings it is not always 
     * necessary for every field in a zone_spec to contain a value. All 
     * zone_specs must have at least ID and GMT offset. Zones that use 
     * daylight savings must have all fields filled except: 
     * STD ABBR, STD NAME, DST NAME. You should take note 
     * that DST ABBR is mandatory for zones that use daylight savings 
     * (see field descriptions for further details).
     *
     * ******* Fields and their description/details ********* 
     *     
     * ID: 
     * Contains the identifying string for the zone_spec. Any string will
     * do as long as it's unique. No two ID's can be the same. 
     *
     * STD ABBR:
     * STD NAME:
     * DST ABBR:
     * DST NAME:
     * These four are all the names and abbreviations used by the time 
     * zone being described. While any string will do in these fields, 
     * care should be taken. These fields hold the strings that will be 
     * used in the output of many of the local_time classes. 
     * Ex:
     * @code
     * time_zone nyc = tz_db.time_zone_from_region("America/New_York");
     * local_time ny_time(date(2004, Aug, 30), IS_DST, nyc);
     * cout << ny_time.to_long_string() << endl;
     * // 2004-Aug-30 00:00:00 Eastern Daylight Time
     * cout << ny_time.to_short_string() << endl;
     * // 2004-Aug-30 00:00:00 EDT
     * @endcode
     *
     * NOTE: The exact format/function names may vary - see local_time 
     * documentation for further details.
     *
     * GMT offset:
     * This is the number of hours added to utc to get the local time 
     * before any daylight savings adjustments are made. Some examples 
     * are: America/New_York offset -5 hours, & Africa/Cairo offset +2 hours.
     * The format must follow the length-of-time format described above.
     *
     * DST adjustment:
     * The amount of time added to gmt_offset when daylight savings is in 
     * effect. The format must follow the length-of-time format described
     * above.
     *
     * DST Start Date rule:
     * This is a specially formatted string that describes the day of year
     * in which the transition take place. It holds three fields of it's own,
     * separated by semicolons. 
     * The first field indicates the "nth" weekday of the month. The possible 
     * values are: 1 (first), 2 (second), 3 (third), 4 (fourth), 5 (fifth), 
     * and -1 (last).
     * The second field indicates the day-of-week from 0-6 (Sun=0).
     * The third field indicates the month from 1-12 (Jan=1).
     * 
     * Examples are: "-1;5;9"="Last Friday of September", 
     * "2;1;3"="Second Monday of March"
     *
     * Start time:
     * Start time is the number of hours past midnight, on the day of the
     * start transition, the transition takes place. More simply put, the 
     * time of day the transition is made (in 24 hours format). The format
     * must follow the length-of-time format described above with the 
     * exception that it must always be positive.
     *
     * DST End date rule:
     * See DST Start date rule. The difference here is this is the day 
     * daylight savings ends (transition to STD).
     *
     * End time:
     * Same as Start time.
     */
    template<class time_zone_type, class rule_type>
    class tz_db_base {
    public:
      /* Having CharT as a template parameter created problems 
       * with posix_time::duration_from_string. Templatizing 
       * duration_from_string was not possible at this time, however, 
       * it should be possible in the future (when poor compilers get 
       * fixed or stop being used). 
       * Since this class was designed to use CharT as a parameter it 
       * is simply typedef'd here to ease converting in back to a 
       * parameter the future */
      typedef char char_type;

      typedef typename time_zone_type::base_type time_zone_base_type;
      typedef typename time_zone_type::time_duration_type time_duration_type;
      typedef time_zone_names_base<char_type> time_zone_names;
      typedef boost::date_time::dst_adjustment_offsets<time_duration_type> dst_adjustment_offsets;
      typedef std::basic_string<char_type> string_type;

      //! Constructs an empty database
      tz_db_base() {}

      //! Process csv data file, may throw exceptions
      /*! May throw bad_field_count exceptions */
      void load_from_stream(std::istream &in)
      {
        std::string buff;
        while( std::getline(in, buff)) {
          boost::trim_right(buff);
          parse_string(buff);
        }
      }

      //! Process csv data file, may throw exceptions
      /*! May throw data_not_accessible, or bad_field_count exceptions */
      void load_from_file(const std::string& pathspec)
      {
        std::string  buff;
        
        std::ifstream ifs(pathspec.c_str());
        if(!ifs){
          boost::throw_exception(data_not_accessible(pathspec));
        }
        std::getline(ifs, buff); // first line is column headings
        this->load_from_stream(ifs);
      }

      //! returns true if record successfully added to map
      /*! Takes a region name in the form of "America/Phoenix", and a 
       * time_zone object for that region. The id string must be a unique 
       * name that does not already exist in the database. */
      bool add_record(const string_type& region, 
                      boost::shared_ptr<time_zone_base_type> tz)
      {
        typename map_type::value_type p(region, tz); 
        return (m_zone_map.insert(p)).second;
      }

      //! Returns a time_zone object built from the specs for the given region
      /*! Returns a time_zone object built from the specs for the given 
       * region. If region does not exist a local_time::record_not_found 
       * exception will be thrown */
      boost::shared_ptr<time_zone_base_type> 
      time_zone_from_region(const string_type& region) const 
      {
        // get the record
        typename map_type::const_iterator record = m_zone_map.find(region);
        if(record == m_zone_map.end()){
          return boost::shared_ptr<time_zone_base_type>(); //null pointer
        }
        return record->second;
      }

      //! Returns a vector of strings holding the time zone regions in the database
      std::vector<std::string> region_list() const
      {
        typedef std::vector<std::string> vector_type;
        vector_type regions;
        typename map_type::const_iterator itr = m_zone_map.begin();
        while(itr != m_zone_map.end()) {
          regions.push_back(itr->first);
          ++itr;
        }
        return regions;
      }
    
    private:
      typedef std::map<string_type, boost::shared_ptr<time_zone_base_type> > map_type;
      map_type m_zone_map;

      // start and end rule are of the same type
      typedef typename rule_type::start_rule::week_num week_num;

      /* TODO: mechanisms need to be put in place to handle different
       * types of rule specs. parse_rules() only handles nth_kday
       * rule types. */
      
      //! parses rule specs for transition day rules
      rule_type* parse_rules(const string_type& sr, const string_type& er) const
      {
        // start and end rule are of the same type, 
        // both are included here for readability
        typedef typename rule_type::start_rule start_rule;
        typedef typename rule_type::end_rule end_rule;
       
        // these are: [start|end] nth, day, month
        int s_nth = 0, s_d = 0, s_m = 0;
        int e_nth = 0, e_d = 0, e_m = 0;
        split_rule_spec(s_nth, s_d, s_m, sr);
        split_rule_spec(e_nth, e_d, e_m, er);
        
        typename start_rule::week_num s_wn, e_wn;
        s_wn = get_week_num(s_nth);
        e_wn = get_week_num(e_nth);
        
        
        return new rule_type(start_rule(s_wn,
                                        static_cast<unsigned short>(s_d),
                                        static_cast<unsigned short>(s_m)),
                             end_rule(e_wn,
                                      static_cast<unsigned short>(e_d),
                                      static_cast<unsigned short>(e_m)));
      }
      //! helper function for parse_rules()
      week_num get_week_num(int nth) const
      {
        typedef typename rule_type::start_rule start_rule;
        switch(nth){
        case 1:
          return start_rule::first;
        case 2:
          return start_rule::second;
        case 3:
          return start_rule::third;
        case 4:
          return start_rule::fourth;
        case 5:
        case -1:
          return start_rule::fifth;
        default:
          // shouldn't get here - add error handling later
          break;
        }
        return start_rule::fifth; // silence warnings
      }
          
      //! splits the [start|end]_date_rule string into 3 ints
      void split_rule_spec(int& nth, int& d, int& m, string_type rule) const
      {
        typedef boost::char_separator<char_type, std::char_traits<char_type> > char_separator_type;
        typedef boost::tokenizer<char_separator_type,
                                 std::basic_string<char_type>::const_iterator,
                                 std::basic_string<char_type> > tokenizer;
        typedef boost::tokenizer<char_separator_type,
                                 std::basic_string<char_type>::const_iterator,
                                 std::basic_string<char_type> >::iterator tokenizer_iterator;
        
        const char_type sep_char[] = { ';', '\0'};
        char_separator_type sep(sep_char);
        tokenizer tokens(rule, sep); // 3 fields

        if ( std::distance ( tokens.begin(), tokens.end ()) != 3 ) {
          std::ostringstream msg;
          msg << "Expecting 3 fields, got " 
              << std::distance ( tokens.begin(), tokens.end ()) 
              << " fields in line: " << rule;
          boost::throw_exception(bad_field_count(msg.str()));
        }

        tokenizer_iterator tok_iter = tokens.begin(); 
        nth = std::atoi(tok_iter->c_str()); ++tok_iter;
        d   = std::atoi(tok_iter->c_str()); ++tok_iter;
        m   = std::atoi(tok_iter->c_str());
      }

     
      //! Take a line from the csv, turn it into a time_zone_type.
      /*! Take a line from the csv, turn it into a time_zone_type,
       * and add it to the map. Zone_specs in csv file are expected to 
       * have eleven fields that describe the time zone. Returns true if 
       * zone_spec successfully added to database */
      bool parse_string(string_type& s)
      {
        std::vector<string_type> result;
        typedef boost::token_iterator_generator<boost::escaped_list_separator<char_type>, string_type::const_iterator, string_type >::type token_iter_type;

        token_iter_type i = boost::make_token_iterator<string_type>(s.begin(), s.end(),boost::escaped_list_separator<char_type>());

        token_iter_type end;
        while (i != end) {
          result.push_back(*i);
          i++;
        }

        enum db_fields { ID, STDABBR, STDNAME, DSTABBR, DSTNAME, GMTOFFSET,
                         DSTADJUST, START_DATE_RULE, START_TIME, END_DATE_RULE,
                         END_TIME, FIELD_COUNT };

        //take a shot at fixing gcc 4.x error
        const unsigned int expected_fields = static_cast<unsigned int>(FIELD_COUNT);
        if (result.size() != expected_fields) { 
          std::ostringstream msg;
          msg << "Expecting " << FIELD_COUNT << " fields, got " 
            << result.size() << " fields in line: " << s;
          boost::throw_exception(bad_field_count(msg.str()));
          BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(return false); // should never reach
        }

        // initializations
        bool has_dst = true; 
        if(result[DSTABBR] == std::string()){
          has_dst = false;
        }


        // start building components of a time_zone
        time_zone_names names(result[STDNAME], result[STDABBR],
                              result[DSTNAME], result[DSTABBR]);

        time_duration_type utc_offset = 
          str_from_delimited_time_duration<time_duration_type,char_type>(result[GMTOFFSET]);
        
        dst_adjustment_offsets adjust(time_duration_type(0,0,0),
                                      time_duration_type(0,0,0),
                                      time_duration_type(0,0,0));

        boost::shared_ptr<rule_type> rules;

        if(has_dst){
          adjust = dst_adjustment_offsets(
                                          str_from_delimited_time_duration<time_duration_type,char_type>(result[DSTADJUST]),
                                          str_from_delimited_time_duration<time_duration_type,char_type>(result[START_TIME]),
                                          str_from_delimited_time_duration<time_duration_type,char_type>(result[END_TIME])
                                          );

          rules = 
            boost::shared_ptr<rule_type>(parse_rules(result[START_DATE_RULE],
                                                     result[END_DATE_RULE]));
        }
        string_type id(result[ID]);
        boost::shared_ptr<time_zone_base_type> zone(new time_zone_type(names, utc_offset, adjust, rules));
        return (add_record(id, zone));
        
      } 
     
    };

} } // namespace

#endif // DATE_TIME_TZ_DB_BASE_HPP__
