
//
// YamlReader.cpp
//

/*
* DISCLAIMER:
* This software was produced by the National Institute of Standards
* and Technology (NIST), an agency of the U.S. government, and by statute is
* not subject to copyright in the United States.  Recipients of this software
* assume all responsibility associated with its operation, modification,
* maintenance, and subsequent redistribution.
*
* See NIST Administration Manual 4.09.07 b and Appendix I.
*/

#include <stdafx.h>
#include <algorithm>
#include <fstream>
#include <streambuf>

#include <boost/lexical_cast.hpp>
#include <boost/spirit/home/classic/actor/assign_actor.hpp>
#include <boost/spirit/home/classic/actor/push_back_actor.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "YamlReader.h"

static std::string Trim (std::string source, std::string delims = " \t\r\n")
{
	std::string result = source.erase(source.find_last_not_of(delims) + 1);
	return result.erase(0, result.find_first_not_of(delims));
}
static std::string StrFormat(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	
	int m;
	size_t n= strlen(format) + 1028;
	std::string tmp(n,'0');	


	// Kind of a bogus way to insure that we don't
	// exceed the limit of our buffer
	while((m=_vsnprintf(&tmp[0], n-1, format, ap))<0)
	{
		n=n+1028;
		tmp.resize(n,'0');
	}
	va_end(ap);
	return tmp.substr(0,m);

}
struct CYamlTokens
{
public:
    enum
    {
        yamlID = 1,
        keyID,
        valueID,
        sectionID,
        expressionID,
        sectionnameID,
        insectionID,
        eolID,
        blanklinesID
    };
};

//  the lexeme directive turns off white space skipping

struct CYamlParser : public GRAMMAR<CYamlParser>, public CYamlTokens
{
    CYamlParser(void) { }
    ~CYamlParser(void) { }
    void Clear ( )
    {
        m_keyname.clear( );
        m_valuestr.clear( );
        m_sections.clear( );
        m_sectionvalues.clear( );
		m_sections.push_back("ROOT");
        m_fullsectionname = "ROOT";
    }

    static void write (const iterator_t first, const iterator_t last)
    {
        std::string str(first, last);
        OutputDebugString(str.c_str( ));
    }

	std::string                        m_fullsectionname;
    std::map<std::string, std::string> m_sectionvalues;
	std::vector<std::string>           m_sections;
	std::string                        m_keyname;
	std::string                        m_valuestr;

	void m_push (const iterator_t first, const iterator_t last)
    {
        std::string str(first, last);
        str              = Trim(str);
        m_fullsectionname += "." + str;

        m_sections.push_back(m_fullsectionname);
    }
	void m_pop (const iterator_t first, const iterator_t last)
    {
       if ( m_fullsectionname.find('.') == std::string::npos )
        {
            return;
        }
        m_fullsectionname
            = m_fullsectionname.substr(0, m_fullsectionname.find_last_of('.'));
    }
	void m_savekey (const iterator_t first, const iterator_t last)
    {
        std::string str(first, last);
        m_keyname = Trim(str);
    }

    void m_savevalue (const iterator_t first, const iterator_t last)
    {
        std::string str(first, last);
        m_valuestr = Trim(str);
    }
	void m_save (const iterator_t first, const iterator_t last)
    {
        std::string str(first, last);
        m_sectionvalues[m_fullsectionname + "." + m_keyname] = m_valuestr;
    }
    template<typename ScannerT>
    struct definition
    {
public:
        const SPIRIT::rule<ScannerT> & start ( ) const { return yaml_start; }
        rule<ScannerT>                                               yaml_start;
        rule<ScannerT, parser_context<>, parser_tag<yamlID> >        yaml;
        rule<ScannerT, parser_context<>, parser_tag<keyID> >         key;
        rule<ScannerT, parser_context<>, parser_tag<valueID> >       value;
        rule<ScannerT, parser_context<>, parser_tag<sectionID> >     section;
        rule<ScannerT, parser_context<>, parser_tag<insectionID> >   insection;
        rule<ScannerT, parser_context<>, parser_tag<expressionID> >  expression;
        rule<ScannerT, parser_context<>, parser_tag<sectionnameID> > sectionname;
        rule<ScannerT, parser_context<>, parser_tag<eolID> >         eol;
        rule<ScannerT, parser_context<>, parser_tag<blanklinesID> >  blankline;
        definition(CYamlParser const & self)
        {
			CYamlParser * ptr=(CYamlParser *)&self;

			boost::function<void (const iterator_t,const iterator_t)>    fcn_pop( boost::bind( &CYamlParser::m_pop, ptr,_1,_2 ) );
			boost::function<void (const iterator_t,const iterator_t)>    fcn_push( boost::bind( &CYamlParser::m_push, ptr,_1,_2 ) );
			boost::function<void (const iterator_t,const iterator_t)>    fcn_key( boost::bind( &CYamlParser::m_savekey, ptr,_1,_2 ) );
			boost::function<void (const iterator_t,const iterator_t)>    fcn_value( boost::bind( &CYamlParser::m_savevalue, ptr,_1,_2 ) );
			boost::function<void (const iterator_t,const iterator_t)>    fcn_save( boost::bind( &CYamlParser::m_save, ptr,_1,_2 ) );
            yaml_start = yaml;
            yaml       = *( expression | section | blankline );
            blankline  = +eol;
            section    = ( sectionname )[fcn_push] >> *( eol ) >> str_p("{") >> *( eol )
                         >> insection >> str_p("}")[fcn_pop ] >> *( eol );
            sectionname = lexeme_d[alnum_p >> *( alnum_p | ch_p('_') )];
            key         = lexeme_d[+( alnum_p | '_' )];
            value       = *( anychar_p - eol ) >> eol;
            eol         = str_p("\n");
            insection   = *( expression | section | blankline );
            expression  = ( ( lexeme_d[+( alnum_p | '_' )] )[fcn_key] >> str_p("=")
                            >> ( *( anychar_p - '\n' ) )[fcn_value] >> ch_p('\n') )[fcn_save];
        }
    };
};



struct yaml3_skip_parser : grammar<yaml3_skip_parser>
{
    template<typename ScannerT>
    struct definition
    {
        definition(yaml3_skip_parser const & /*self*/)
        {
            skip = ( ch_p(' ') | ch_p('\t') ) | "#" >> *( anychar_p - '\n' ) >> '\n';
        }

        rule<ScannerT>         skip;
        rule<ScannerT> const & start ( ) const { return skip; }
    };
};


//////////////////////////////////////////////////////////////////////////////////////
YamlReader::YamlReader( ) 
{ 
	parser = boost::shared_ptr<CYamlParser>(new CYamlParser( )); 
}

//////////////////////////////////////////////////////////////////////////////////////
int YamlReader::Load (std::string str)
{
    // Line feed is used as delimiter, so can't be in skip
    // Clueless on how iterator_t work - either const char *, or string, or spirit
    yaml3_skip_parser skip3;
    const char *      str1 = str.c_str( );
    const char *      str2 = str.c_str( ) + str.size( );
    iterator_t        first(str1, str2, (const char *) str.c_str( ));
    iterator_t        last;

    parser->Clear( );
    parse_info<iterator_t> info = parse(first, last, *parser, skip3);

    return info.full;
}
//////////////////////////////////////////////////////////////////////////////////////
int YamlReader::LoadFromFile (std::string filename)
{
	std::ifstream t(filename);
	std::string contents((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	return Load(contents);
}
//////////////////////////////////////////////////////////////////////////////////////
std::string YamlReader::Find (std::string key)
{
    std::map<std::string, std::string>::iterator it
        = parser->m_sectionvalues.find(key);

    if ( it != parser->m_sectionvalues.end( ) )
    {
        return ( *it ).second;
    }
    return "";
}
//////////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> YamlReader::Sections ( ) 
{ 
	return parser->m_sections; 
}
//////////////////////////////////////////////////////////////////////////////////////
std::map<std::string, std::string> YamlReader::Section (std::string section)
{
    std::map<std::string, std::string> pairs;
    std::vector<std::string>           keys = Keys(section);

    for ( size_t i = 0; i < keys.size( ); i++ )
    {
        std::string value = Find(keys[i]);
        pairs[keys[i]] = value;
    }
    return pairs;
}
//////////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> YamlReader::Keys (std::string section)
{
    std::vector<std::string> keys;

    for ( std::map<std::string, std::string>::iterator it
              = parser->m_sectionvalues.begin( );
          it != parser->m_sectionvalues.end( ); it++ )
    {
        std::string fullkeyname = ( *it ).first;
        std::string sectionname
            = fullkeyname.substr(0, fullkeyname.find_last_of('.'));
        std::string keyname = fullkeyname.substr(fullkeyname.find_last_of('.') + 1);

        if ( sectionname == section )
        {
            keys.push_back(keyname);
        }
    }
    return keys;
}
//////////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> YamlReader::Subsections (std::string section)
{
    std::vector<std::string> subsections;
    std::vector<std::string> allsubsections;
    std::vector<std::string> allsections = parser->m_sections;

    for ( size_t i = 0; i < allsections.size( ); i++ )
    {
        if ( allsections[i].find(section) != std::string::npos )
        {
            if ( allsections[i] == section )               /** skip matching section */
            {
                continue;
            }
            allsubsections.push_back(allsections[i]);
        }
    }

    // now we have all branches below current section - need to prune to children
    size_t n = std::count(section.begin( ), section.end( ), '.') + 1;

    for ( size_t i = 0; i < allsubsections.size( ); i++ )
    {
        size_t level
            = std::count(allsubsections[i].begin( ), allsubsections[i].end( ), '.');

        if ( level == n )
        {
            subsections.push_back(allsubsections[i]);
        }
    }
    return subsections;
}


int YamlReader::AddSection(std::string section)
{
	// Must start with ROOT. as new section otherwise error
	if(section.compare( 0, strlen("ROOT."), "ROOT.") != 0)
		return -1;

	// insure all pre susections exists  ROOT.X.Y.Z i.e., X and Y
	// or create
	std::string fullkeyname(section);
	std::string partialsection;
	while(!fullkeyname.empty())
	{
		partialsection += fullkeyname.substr(0, fullkeyname.find_first_of('.'));
		size_t n = fullkeyname.find_first_of('.');
		if( n != std::string::npos)
			fullkeyname = fullkeyname.substr(fullkeyname.find_first_of('.')+1);
		else
			fullkeyname.clear();
		if(std::find(parser->m_sections.begin(), parser->m_sections.end(), partialsection) == parser->m_sections.end())
			parser->m_sections.push_back(partialsection);
		partialsection += ".";
	}

	// This is redundant, as new section should already be added
	if(std::find(parser->m_sections.begin(), parser->m_sections.end(), section) == parser->m_sections.end())
		parser->m_sections.push_back(section);

	return 0;
}

int YamlReader::SetKeyValue(std::string key, std::string value)
{
	// insure all pre subsections exists  ROOT.X.Y.Z i.e., X and Y
	// or create
	// Must start with ROOT. as new section otherwise error
	if(key.compare( 0, strlen("ROOT."), "ROOT.") != 0)
		return -1;

		// insure all pre susections exists  ROOT.X.Y.Z i.e., X and Y
	// or create
	std::string fullkeyname(key);
	std::string partialsection;
	while(!fullkeyname.empty())
	{
		partialsection += fullkeyname.substr(0, fullkeyname.find_first_of('.'));
		size_t n = fullkeyname.find_first_of('.');
		if( n != std::string::npos)
			fullkeyname = fullkeyname.substr(fullkeyname.find_first_of('.')+1);
		else
		{
			fullkeyname.clear();
			continue;
		}
		if(std::find(parser->m_sections.begin(), parser->m_sections.end(), partialsection) == parser->m_sections.end())
			parser->m_sections.push_back(partialsection);
		partialsection += ".";
	}

	// Now add key value
	parser->m_sectionvalues[key] = value;
	return 0;
}

void YamlReader::Output (std::string branch, std::stringstream & os, int indent)
{

    std::vector<std::string> keys = Keys(branch);

    for ( size_t i = 0; i < keys.size( ); i++ )
    {
		std::string key,value;
		key=keys[i];
		value = Find(branch + "." + keys[i]);
		os << StrFormat("%*s", indent,"") << key << "=" << value << "\n";
    }

	// Now parse subbranches
    std::vector<std::string> sections = Subsections(branch);

    for ( size_t i = 0; i < sections.size( ); i++ )
	{
		std::string subbranch;
		std::size_t found = sections[i].find_last_of(".");
		if(found== std::string::npos)
			assert(0);
		subbranch=sections[i].substr(found+1);
		os << StrFormat("%*s", indent,"") << subbranch << "\n";
		os << StrFormat("%*s", indent,"") << "{\n";
		Output(sections[i], os,indent+4);
		os << StrFormat("%*s", indent,"") << "}\n";
	}
}
//////////////////////////////////////////////////////////////////////////////////////
std::string YamlReader::ToString ( )
{
	std::stringstream ss;
    Output("ROOT", ss);
	return ss.str();
}