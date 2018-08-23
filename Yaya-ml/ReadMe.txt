


To add boost property support:

Declare in YamlReader: (appears as if spirit include property tree)
    //////////////////////////////////////
    void                               ParseIntoBoostPropertyTreee (std::string branch, ptree & pt);
    ptree                              pt;                 /**< boost property tree parse tree */

	/**
	* \brief parses all the fully qualified section names and key/value pairs into
	* boost property tree equivalent.
	* Unclear if this is even useful.
	*/
	void                               ParseIntoBoostPropertyTreee ( );


then add  to source code:

//////////////////////////////////////////////////////////////////////////////////////
void YamlReader::ParseIntoBoostPropertyTreee (std::string branch, ptree & pt)
{
    std::string data;                                      // data associated with the node
    std::list<std::pair<std::string, ptree> >
    children;                                              // ordered list of named children
    pt.data = branch;
    std::vector<std::string> keys = Keys(branch);

    for ( size_t i = 0; i < keys.size( ); i++ )
    {
        ptree p;
        p.data = Find(branch + "." + keys[i]);
        pt.children.insert(pt.children.begin( ),
                           std::pair<std::string, ptree>(keys[i], p));
    }

    std::vector<std::string> sections = Subsections(branch);

    for ( size_t i = 0; i < sections.size( ); i++ )
    {
        std::string subbranch = branch + "." + sections[i];
        ptree       p;
        pt.children.insert(pt.children.begin( ),
                           std::pair<std::string, ptree>(subbranch, p));
        ParseIntoBoostPropertyTreee(subbranch, p);
    }
}
//////////////////////////////////////////////////////////////////////////////////////
void YamlReader::ParseIntoBoostPropertyTreee ( )
{
    pt.children.clear( );
    ParseIntoBoostPropertyTreee("ROOT", this->pt);
}
