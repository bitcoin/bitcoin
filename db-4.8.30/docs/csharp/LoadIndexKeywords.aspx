<%@ Page Language="C#" EnableViewState="False" %>

<script runat="server">
//=============================================================================
// System  : Sandcastle Help File Builder
// File    : LoadIndexKeywords.aspx
// Author  : Eric Woodruff  (Eric@EWoodruff.us) from code by Ferdinand Prantl
// Updated : 04/01/2008
// Note    : Copyright 2008, Eric Woodruff, All rights reserved
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
// 1.6.0.7  04/01/2008  EFW  Created the code
//=============================================================================

/// <summary>
/// Render the keyword index
/// </summary>
/// <param name="writer">The writer to which the results are written</param>
protected override void Render(HtmlTextWriter writer)
{
    XmlDocument ki;
    XmlNode root, node;
    StringBuilder sb = new StringBuilder(10240);
    int startIndex = 0, endIndex;
    string url, target;

    ki = new XmlDocument();
    ki.Load(Server.MapPath("WebKI.xml"));
    root = ki.SelectSingleNode("HelpKI");

    if(Request.QueryString["StartIndex"] != null)
        startIndex = Convert.ToInt32(Request.QueryString["StartIndex"]) * 128;

    endIndex = startIndex + 128;

    if(endIndex > root.ChildNodes.Count)
        endIndex = root.ChildNodes.Count;

    if(startIndex > 0)
    {
        sb.Append("<div class=\"IndexItem\">\r\n" +
            "<span>&nbsp;</span><a class=\"UnselectedNode\" " +
            "onclick=\"javascript: return ChangeIndexPage(-1);\" " +
            "href=\"#\"><b><< Previous page</b></a>\r\n</div>\r\n");
    }

    while(startIndex < endIndex)
    {
        node = root.ChildNodes[startIndex];

        if(node.Attributes["Url"] == null)
        {
            url = "#";
            target = String.Empty;
        }
        else
        {
            url = node.Attributes["Url"].Value;
            target = " target=\"TopicContent\"";
        }

        sb.AppendFormat("<div class=\"IndexItem\">\r\n" +
            "<span>&nbsp;</span><a class=\"UnselectedNode\" " +
            "onclick=\"javascript: return SelectIndexNode(this);\" " +
            "href=\"{0}\"{1}>{2}</a>\r\n", url, target,
            HttpUtility.HtmlEncode(node.Attributes["Title"].Value));

        if(node.ChildNodes.Count != 0)
            foreach(XmlNode subNode in node.ChildNodes)
                sb.AppendFormat("<div class=\"IndexSubItem\">\r\n" +
                    "<img src=\"Item.gif\"/><a class=\"UnselectedNode\" " +
                    "onclick=\"javascript: return SelectIndexNode(this);\" " +
                    "href=\"{0}\" target=\"TopicContent\">{1}</a>\r\n</div>\r\n",
                    subNode.Attributes["Url"].Value,
                    HttpUtility.HtmlEncode(subNode.Attributes["Title"].Value));

        sb.Append("</div>\r\n");

        startIndex++;
    }

    if(startIndex < root.ChildNodes.Count)
        sb.Append("<div class=\"IndexItem\">\r\n" +
            "<span>&nbsp;</span><a class=\"UnselectedNode\" " +
            "onclick=\"javascript: return ChangeIndexPage(1);\" " +
            "href=\"#\"><b>Next page >></b></a>\r\n</div>\r\n");

    writer.Write(sb.ToString());
}

</script>
