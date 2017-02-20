<%@ Page Language="C#" EnableViewState="False" %>

<script runat="server">
//=============================================================================
// System  : Sandcastle Help File Builder
// File    : FillNode.aspx
// Author  : Eric Woodruff  (Eric@EWoodruff.us)
// Updated : 04/02/2008
// Note    : Copyright 2007-2008, Eric Woodruff, All rights reserved
// Compiler: Microsoft C#
//
// This file contains the code used to dynamically load a parent node with its
// child table of content nodes when first expanded.
//
// This code is published under the Microsoft Public License (Ms-PL).  A copy
// of the license should be distributed with the code.  It can also be found
// at the project website: http://www.CodePlex.com/SHFB.   This notice, the
// author's name, and all copyright notices must remain intact in all
// applications, documentation, and source files.
//
// Version     Date     Who  Comments
// ============================================================================
// 1.5.0.0  06/21/2007  EFW  Created the code
//=============================================================================

protected override void Render(HtmlTextWriter writer)
{
    StringBuilder sb = new StringBuilder(10240);
    string id, url, target, title;

    XPathDocument toc = new XPathDocument(Server.MapPath("WebTOC.xml"));
    XPathNavigator navToc = toc.CreateNavigator();

    // The ID to use should be passed in the query string
    XPathNodeIterator root = navToc.Select("//HelpTOCNode[@Id='" +
        this.Request.QueryString["Id"] + "']/*");

    if(root.Count == 0)
    {
        writer.Write("<b>TOC node not found!</b>");
        return;
    }

    foreach(XPathNavigator node in root)
    {
        if(node.HasChildren)
        {
            // Write out a parent TOC entry
            id = node.GetAttribute("Id", String.Empty);
            title = node.GetAttribute("Title", String.Empty);
            url = node.GetAttribute("Url", String.Empty);

            if(!String.IsNullOrEmpty(url))
                target = " target=\"TopicContent\"";
            else
            {
                url = "#";
                target = String.Empty;
            }

            sb.AppendFormat("<div class=\"TreeNode\">\r\n" +
                "<img class=\"TreeNodeImg\" " +
                "onclick=\"javascript: Toggle(this);\" " +
                "src=\"Collapsed.gif\"/><a class=\"UnselectedNode\" " +
                "onclick=\"javascript: return Expand(this);\" " +
                "href=\"{0}\"{1}>{2}</a>\r\n" +
                "<div id=\"{3}\" class=\"Hidden\"></div>\r\n</div>\r\n",
                url, target, HttpUtility.HtmlEncode(title), id);
        }
        else
        {
            title = node.GetAttribute("Title", String.Empty);
            url = node.GetAttribute("Url", String.Empty);

            if(String.IsNullOrEmpty(url))
                url = "about:blank";

            // Write out a TOC entry that has no children
            sb.AppendFormat("<div class=\"TreeItem\">\r\n" +
                "<img src=\"Item.gif\"/>" +
                "<a class=\"UnselectedNode\" " +
                "onclick=\"javascript: return SelectNode(this);\" " +
                "href=\"{0}\" target=\"TopicContent\">{1}</a>\r\n" +
                "</div>\r\n", url, HttpUtility.HtmlEncode(title));
        }
    }

    writer.Write(sb.ToString());
}
</script>
