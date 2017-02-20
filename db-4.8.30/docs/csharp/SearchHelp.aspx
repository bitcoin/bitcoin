<%@ Page Language="C#" EnableViewState="False" %>

<script runat="server">
//=============================================================================
// System  : Sandcastle Help File Builder
// File    : SearchHelp.aspx
// Author  : Eric Woodruff  (Eric@EWoodruff.us)
// Updated : 07/03/2007
// Note    : Copyright 2007, Eric Woodruff, All rights reserved
// Compiler: Microsoft C#
//
// This file contains the code used to search for keywords within the help
// topics using the full-text index files created by the help file builder.
//
// This code is published under the Microsoft Public License (Ms-PL).  A copy
// of the license should be distributed with the code.  It can also be found
// at the project website: http://www.CodePlex.com/SHFB.   This notice, the
// author's name, and all copyright notices must remain intact in all
// applications, documentation, and source files.
//
// Version     Date     Who  Comments
// ============================================================================
// 1.5.0.0  06/24/2007  EFW  Created the code
//=============================================================================

private class Ranking
{
    public string Filename, PageTitle;
    public int Rank;
    
    public Ranking(string file, string title, int rank)
    {
        Filename = file;
        PageTitle = title;
        Rank = rank;
    }
}    

/// <summary>
/// Render the search results
/// </summary>
/// <param name="writer">The writer to which the results are written</param>
protected override void Render(HtmlTextWriter writer)
{
    FileStream fs = null;
    BinaryFormatter bf;
    string searchText, ftiFile;
    char letter;
    bool sortByTitle = false;

    // The keywords for which to search should be passed in the query string
    searchText = this.Request.QueryString["Keywords"];

    if(String.IsNullOrEmpty(searchText))
    {
        writer.Write("<b class=\"PaddedText\">Nothing found</b>");
        return;
    }

    // An optional SortByTitle option can also be specified
    if(this.Request.QueryString["SortByTitle"] != null)
        sortByTitle = Convert.ToBoolean(this.Request.QueryString["SortByTitle"]);

    List<string> keywords = this.ParseKeywords(searchText);
    List<char> letters = new List<char>();
    List<string> fileList;
    Dictionary<string, List<long>> ftiWords, wordDictionary =
        new Dictionary<string,List<long>>();

    try
    {
        // Load the file index
        fs = new FileStream(Server.MapPath("fti/FTI_Files.bin"), FileMode.Open,
            FileAccess.Read);
        bf = new BinaryFormatter();
        fileList = (List<string>)bf.Deserialize(fs);
        fs.Close();

        // Load the required word index files
        foreach(string word in keywords)
        {
            letter = word[0];
            
            if(!letters.Contains(letter))
            {
                letters.Add(letter);
                ftiFile = Server.MapPath(String.Format(
                    CultureInfo.InvariantCulture, "fti/FTI_{0}.bin", (int)letter));

                if(File.Exists(ftiFile))
                {
                    fs = new FileStream(ftiFile, FileMode.Open, FileAccess.Read);
                    ftiWords = (Dictionary<string, List<long>>)bf.Deserialize(fs);
                    fs.Close();

                    foreach(string ftiWord in ftiWords.Keys)
                        wordDictionary.Add(ftiWord, ftiWords[ftiWord]);
                }
            }
        }
    }
    finally
    {
        if(fs != null && fs.CanRead)
            fs.Close();
    }

    // Perform the search and return the results as a block of HTML
    writer.Write(this.Search(keywords, fileList, wordDictionary, sortByTitle));
}

/// <summary>
/// Split the search text up into keywords
/// </summary>
/// <param name="keywords">The keywords to parse</param>
/// <returns>A list containing the words for which to search</returns>
private List<string> ParseKeywords(string keywords)
{
    List<string> keywordList = new List<string>();
    string checkWord;
    string[] words = Regex.Split(keywords, @"\W+");

    foreach(string word in words)
    {
        checkWord = word.ToLower(CultureInfo.InvariantCulture);
        
        if(checkWord.Length > 2 && !Char.IsDigit(checkWord[0]) &&
          !keywordList.Contains(checkWord))
            keywordList.Add(checkWord);
    }

    return keywordList;
}

/// <summary>
/// Search for the specified keywords and return the results as a block of
/// HTML.
/// </summary>
/// <param name="keywords">The keywords for which to search</param>
/// <param name="fileInfo">The file list</param>
/// <param name="wordDictionary">The dictionary used to find the words</param>
/// <param name="sortByTitle">True to sort by title, false to sort by
/// ranking</param>
/// <returns>A block of HTML representing the search results.</returns>
private string Search(List<string> keywords, List<string> fileInfo,
    Dictionary<string, List<long>> wordDictionary, bool sortByTitle)
{
    StringBuilder sb = new StringBuilder(10240);
    Dictionary<string, List<long>> matches = new Dictionary<string, List<long>>();
    List<long> occurrences;
    List<int> matchingFileIndices = new List<int>(),
        occurrenceIndices = new List<int>();
    List<Ranking> rankings = new List<Ranking>();

    string filename, title;
    string[] fileIndex;
    bool isFirst = true;
    int idx, wordCount, matchCount;

// TODO: Support boolean operators (AND, OR and maybe NOT)
        
    foreach(string word in keywords)
    {
        if(!wordDictionary.TryGetValue(word, out occurrences))
            return "<b class=\"PaddedText\">Nothing found</b>";

        matches.Add(word, occurrences);
        occurrenceIndices.Clear();

        // Get a list of the file indices for this match
        foreach(long entry in occurrences)
            occurrenceIndices.Add((int)(entry >> 16));
            
        if(isFirst)
        {
            isFirst = false;
            matchingFileIndices.AddRange(occurrenceIndices);
        }
        else
        {
            // After the first match, remove files that do not appear for
            // all found keywords.
            for(idx = 0; idx < matchingFileIndices.Count; idx++)
                if(!occurrenceIndices.Contains(matchingFileIndices[idx]))
                {
                    matchingFileIndices.RemoveAt(idx);
                    idx--;
                }
        }
    }

    if(matchingFileIndices.Count == 0)
        return "<b class=\"PaddedText\">Nothing found</b>";

    // Rank the files based on the number of times the words occurs
    foreach(int index in matchingFileIndices)
    {
        // Split out the title, filename, and word count
        fileIndex = fileInfo[index].Split('\x0');

        title = fileIndex[0];
        filename = fileIndex[1];
        wordCount = Convert.ToInt32(fileIndex[2]);
        matchCount = 0;
        
        foreach(string word in keywords)
        {
            occurrences = matches[word];

            foreach(long entry in occurrences)
                if((int)(entry >> 16) == index)
                    matchCount += (int)(entry & 0xFFFF);
        }

        rankings.Add(new Ranking(filename, title, matchCount * 1000 / wordCount));
    }

    // Sort by rank in descending order or by page title in ascending order
    rankings.Sort(
        delegate(Ranking x, Ranking y)
        {
            if(!sortByTitle)
                return y.Rank - x.Rank;

            return x.PageTitle.CompareTo(y.PageTitle);
        });

    // Format the file list and return the results
    foreach(Ranking r in rankings)
        sb.AppendFormat("<div class=\"TreeItem\">\r\n<img src=\"Item.gif\"/>" +
            "<a class=\"UnselectedNode\" target=\"TopicContent\" " +
            "href=\"{0}\" onclick=\"javascript: SelectSearchNode(this);\">" +
            "{1}</a>\r\n</div>\r\n", r.Filename, r.PageTitle);

    // Return the keywords used as well in a hidden span
    sb.AppendFormat("<span id=\"SearchKeywords\" style=\"display: none\">{0}</span>",
        String.Join(" ", keywords.ToArray()));

    return sb.ToString();
}

</script>
