//=============================================================================
// System  : Sandcastle Help File Builder
// File    : TOC.js
// Author  : Eric Woodruff  (Eric@EWoodruff.us)
// Updated : 04/11/2008
// Note    : Copyright 2006-2008, Eric Woodruff, All rights reserved
// Compiler: JavaScript
//
// This file contains the methods necessary to implement a simple tree view
// for the table of content with a resizable splitter and Ajax support to
// load tree nodes on demand.  It also contains the script necessary to do
// full-text searches.
//
// This code is published under the Microsoft Public License (Ms-PL).  A copy
// of the license should be distributed with the code.  It can also be found
// at the project website: http://www.CodePlex.com/SHFB.   This notice, the
// author's name, and all copyright notices must remain intact in all
// applications, documentation, and source files.
//
// Version     Date     Who  Comments
// ============================================================================
// 1.3.0.0  09/12/2006  EFW  Created the code
// 1.4.0.2  06/15/2007  EFW  Reworked to get rid of frame set and to add
//                           support for Ajax to load tree nodes on demand.
// 1.5.0.0  06/24/2007  EFW  Added full-text search capabilities
// 1.6.0.7  04/01/2008  EFW  Merged changes from Ferdinand Prantl to add a
//                           website keyword index.  Added support for "topic"
//                           query string option.
//=============================================================================

// IE flag
var isIE = (navigator.userAgent.indexOf("MSIE") >= 0);

// Minimum width of the TOC div
var minWidth = 100;

// Elements and sizing info
var divTOC, divSizer, topicContent, divNavOpts, divSearchOpts, divSearchResults,
    divIndexOpts, divIndexResults, divTree, docBody, maxWidth, offset,
    txtSearchText, chkSortByTitle;

// Last node selected
var lastNode, lastSearchNode, lastIndexNode;

// Last page with keyword index
var currentIndexPage = 0;

//============================================================================

// Initialize the tree view and resize the content
function Initialize()
{
    docBody = document.getElementsByTagName("body")[0];
    divTOC = document.getElementById("TOCDiv");
    divSizer = document.getElementById("TOCSizer");
    topicContent = document.getElementById("TopicContent");
    divNavOpts = document.getElementById("divNavOpts");
    divSearchOpts = document.getElementById("divSearchOpts");
    divSearchResults = document.getElementById("divSearchResults");
    divIndexOpts = document.getElementById("divIndexOpts");
    divIndexResults = document.getElementById("divIndexResults");
    divTree = document.getElementById("divTree");
    txtSearchText = document.getElementById("txtSearchText");
    chkSortByTitle = document.getElementById("chkSortByTitle");

    // The sizes are bit off in FireFox
    if(!isIE)
        divNavOpts.style.width = divSearchOpts.style.width =
            divIndexOpts.style.width = 292;

    ResizeTree();
    SyncTOC();

    // Use an alternate default page if a topic is specified in
    // the query string.
    var queryString = document.location.search;

    if(queryString != "")
    {
        var idx, options = queryString.split(/[\?\=\&]/);

        for(idx = 0; idx < options.length; idx++)
            if(options[idx] == "topic" && idx + 1 < options.length)
            {
                topicContent.src = options[idx + 1];
                break;
            }
    }
}

//============================================================================
// Navigation and expand/collaps code

// Synchronize the table of content with the selected page if possible
function SyncTOC()
{
    var idx, anchor, base, href, url, anchors, treeNode, saveNode;

    base = window.location.href;
    base = base.substr(0, base.lastIndexOf("/") + 1);

    if(base.substr(0, 5) == "file:" && base.substr(0, 8) != "file:///")
        base = base.replace("file://", "file:///");

    url = GetCurrentUrl();
    if(url == "")
        return false;

    if(url.substr(0, 5) == "file:" && url.substr(0, 8) != "file:///")
        url = url.replace("file://", "file:///");

    while(true)
    {
        anchors = divTree.getElementsByTagName("A");
        anchor = null;

        for(idx = 0; idx < anchors.length; idx++)
        {
            href = anchors[idx].href;

            if(href.substring(0, 7) != 'http://' &&
              href.substring(0, 8) != 'https://' &&
              href.substring(0, 7) != 'file://')
                href = base + href;

            if(href == url)
            {
                anchor = anchors[idx];
                break;
            }
        }

        if(anchor == null)
        {
            // If it contains a "#", strip anything after that and try again
            if(url.indexOf("#") != -1)
            {
                url = url.substr(0, url.indexOf("#"));
                continue;
            }

            return;
        }

        break;
    }

    // If found, select it and find the parent tree node
    SelectNode(anchor);
    saveNode = anchor;
    lastNode = null;

    while(anchor != null)
    {
        if(anchor.className == "TreeNode")
        {
            treeNode = anchor;
            break;
        }

        anchor = anchor.parentNode;
    }

    // Expand it and all of its parents
    while(anchor != null)
    {
        Expand(anchor);

        anchor = anchor.parentNode;

        while(anchor != null)
        {
            if(anchor.className == "TreeNode")
                break;

            anchor = anchor.parentNode;
        }
    }

    lastNode = saveNode;

    // Scroll the node into view
    var windowTop = lastNode.offsetTop - divTree.offsetTop - divTree.scrollTop;
    var windowBottom = divTree.clientHeight - windowTop - lastNode.offsetHeight;

    if(windowTop < 0)
        divTree.scrollTop += windowTop - 30;
    else
        if(windowBottom < 0)
            divTree.scrollTop -= windowBottom - 30;
}

// Get the currently loaded URL from the IFRAME
function GetCurrentUrl()
{
    var base, url = "";

    try
    {
        url = window.frames["TopicContent"].document.URL.replace(/\\/g, "/");
    }
    catch(e)
    {
        // If this happens the user probably navigated to another frameset
        // that didn't make itself the topmost frameset and we don't have
        // control of the other frame anymore.  In that case, just reload
        // our index page.
        base = window.location.href;
        base = base.substr(0, base.lastIndexOf("/") + 1);

        if(base.substr(0, 5) == "file:" && base.substr(0, 8) != "file:///")
            base = base.replace("file://", "file:///");

        if(base.substr(0, 5) == "file:")
            top.location.href = base + "Index.html";
        else
            top.location.href = base + "Index.aspx";
    }

    return url;
}

// Expand or collapse all nodes
function ExpandOrCollapseAll(expandNodes)
{
    var divIdx, childIdx, img, divs = document.getElementsByTagName("DIV");
    var childNodes, child, div, link, img;

    for(divIdx = 0; divIdx < divs.length; divIdx++)
        if(divs[divIdx].className == "Hidden" ||
          divs[divIdx].className == "Visible")
        {
            childNodes = divs[divIdx].parentNode.childNodes;

            for(childIdx = 0; childIdx < childNodes.length; childIdx++)
            {
                child = childNodes[childIdx];

                if(child.className == "TreeNodeImg")
                    img = child;

                if(child.className == "Hidden" || child.className == "Visible")
                {
                    div = child;
                    break;
                }
            }

            if(div.className == "Visible" && !expandNodes)
            {
                div.className = "Hidden";
                img.src = "Collapsed.gif";
            }
            else
                if(div.className == "Hidden" && expandNodes)
                {
                    div.className = "Visible";
                    img.src = "Expanded.gif";

                    if(div.innerHTML == "")
                        FillNode(div, true)
                }
        }
}

// Toggle the state of the specified node
function Toggle(node)
{
    var i, childNodes, child, div, link;

    childNodes = node.parentNode.childNodes;

    for(i = 0; i < childNodes.length; i++)
    {
        child = childNodes[i];

        if(child.className == "Hidden" || child.className == "Visible")
        {
            div = child;
            break;
        }
    }

    if(div.className == "Visible")
    {
        div.className = "Hidden";
        node.src = "Collapsed.gif";
    }
    else
    {
        div.className = "Visible";
        node.src = "Expanded.gif";

        if(div.innerHTML == "")
            FillNode(div, false)
    }
}

// Expand the selected node
function Expand(node)
{
    var i, childNodes, child, div, img;

    // If not valid, don't bother
    if(GetCurrentUrl() == "")
        return false;

    if(node.tagName == "A")
        childNodes = node.parentNode.childNodes;
    else
        childNodes = node.childNodes;

    for(i = 0; i < childNodes.length; i++)
    {
        child = childNodes[i];

        if(child.className == "TreeNodeImg")
            img = child;

        if(child.className == "Hidden" || child.className == "Visible")
        {
            div = child;
            break;
        }
    }

    if(lastNode != null)
        lastNode.className = "UnselectedNode";

    div.className = "Visible";
    img.src = "Expanded.gif";

    if(node.tagName == "A")
    {
        node.className = "SelectedNode";
        lastNode = node;
    }

    if(div.innerHTML == "")
        FillNode(div, false)

    return true;
}

// Set the style of the specified node to "selected"
function SelectNode(node)
{
    // If not valid, don't bother
    if(GetCurrentUrl() == "")
        return false;

    if(lastNode != null)
        lastNode.className = "UnselectedNode";

    node.className = "SelectedNode";
    lastNode = node;

    return true;
}

//============================================================================
// Ajax-related code used to fill the tree nodes on demand

function GetXmlHttpRequest()
{
    var xmlHttp = null;

    // If IE7, Mozilla, Safari, etc., use the native object.
    // Otherwise, use the ActiveX control for IE5.x and IE6.
    if(window.XMLHttpRequest)
        xmlHttp = new XMLHttpRequest();
    else
        if(window.ActiveXObject)
            xmlHttp = new ActiveXObject("MSXML2.XMLHTTP.3.0");

    return xmlHttp;
}

// Perform an AJAX-style request for the contents of a node and put the
// contents into the empty div.
function FillNode(div, expandChildren)
{
    var xmlHttp = GetXmlHttpRequest(), now = new Date();

    if(xmlHttp == null)
    {
        div.innerHTML = "<b>XML HTTP request not supported!</b>";
        return;
    }

    div.innerHTML = "Loading...";

    // Add a unique hash to ensure it doesn't use cached results
    xmlHttp.open("GET", "FillNode.aspx?Id=" + div.id + "&hash=" +
        now.getTime(), true);

    xmlHttp.onreadystatechange = function()
    {
        if(xmlHttp.readyState == 4)
        {
            div.innerHTML = xmlHttp.responseText;

            if(expandChildren)
                ExpandOrCollapseAll(true);
        }
    }

    xmlHttp.send(null)
}

//============================================================================
// Resizing code

// Resize the tree div so that it fills the document body
function ResizeTree()
{
    var y, newHeight;

    if(self.innerHeight)    // All but IE
        y = self.innerHeight;
    else    // IE - Strict
        if(document.documentElement && document.documentElement.clientHeight)
            y = document.documentElement.clientHeight;
        else    // Everything else
            if(document.body)
                y = document.body.clientHeight;

    newHeight = y - parseInt(divNavOpts.style.height, 10) - 6;

    if(newHeight < 50)
        newHeight = 50;

    divTree.style.height = newHeight;

    newHeight = y - parseInt(divSearchOpts.style.height, 10) - 6;

    if(newHeight < 100)
        newHeight = 100;

    divSearchResults.style.height = newHeight;

    newHeight = y - parseInt(divIndexOpts.style.height, 10) - 6;

    if(newHeight < 25)
        newHeight = 25;

    divIndexResults.style.height = newHeight;

    // Resize the content div
    ResizeContent();
}

// Resize the content div
function ResizeContent()
{
    if(isIE)
        maxWidth = docBody.clientWidth;
    else
        maxWidth = docBody.clientWidth - 4;

    topicContent.style.width = maxWidth - (divSizer.offsetLeft +
        divSizer.offsetWidth);
    maxWidth -= minWidth;
}

// This is called to prepare for dragging the sizer div
function OnMouseDown(event)
{
    var x;

    // Make sure the splitter is at the top of the z-index
    divSizer.style.zIndex = 5000;

    // The content is in an IFRAME which steals mouse events so
    // hide it while resizing.
    topicContent.style.display = "none";

    if(isIE)
        x = window.event.clientX + document.documentElement.scrollLeft +
            document.body.scrollLeft;
    else
        x = event.clientX + window.scrollX;

    // Save starting offset
    offset = parseInt(divSizer.style.left, 10);

    if(isNaN(offset))
        offset = 0;

    offset -= x;

    if(isIE)
    {
        document.attachEvent("onmousemove", OnMouseMove);
        document.attachEvent("onmouseup", OnMouseUp);
        window.event.cancelBubble = true;
        window.event.returnValue = false;
    }
    else
    {
        document.addEventListener("mousemove", OnMouseMove, true);
        document.addEventListener("mouseup", OnMouseUp, true);
        event.preventDefault();
    }
}

// Resize the TOC and content divs as the sizer is dragged
function OnMouseMove(event)
{
    var x, pos;

    // Get cursor position with respect to the page
    if(isIE)
        x = window.event.clientX + document.documentElement.scrollLeft +
            document.body.scrollLeft;
    else
        x = event.clientX + window.scrollX;

    left = offset + x;

    // Adjusts the width of the TOC divs
    pos = (event.clientX > maxWidth) ? maxWidth :
        (event.clientX < minWidth) ? minWidth : event.clientX;

    divTOC.style.width = divSearchResults.style.width =
        divIndexResults.style.width = divTree.style.width = pos;

    if(!isIE)
        pos -= 8;

    divNavOpts.style.width = divSearchOpts.style.width =
        divIndexOpts.style.width = pos;

    // Resize the content div to fit in the remaining space
    ResizeContent();
}

// Finish the drag operation when the mouse button is released
function OnMouseUp(event)
{
    if(isIE)
    {
        document.detachEvent("onmousemove", OnMouseMove);
        document.detachEvent("onmouseup", OnMouseUp);
    }
    else
    {
        document.removeEventListener("mousemove", OnMouseMove, true);
        document.removeEventListener("mouseup", OnMouseUp, true);
    }

    // Show the content div again
    topicContent.style.display = "inline";
}

//============================================================================
// Search code

function ShowHideSearch(show)
{
    if(show)
    {
        divNavOpts.style.display = divTree.style.display = "none";
        divSearchOpts.style.display = divSearchResults.style.display = "";
    }
    else
    {
        divSearchOpts.style.display = divSearchResults.style.display = "none";
        divNavOpts.style.display = divTree.style.display = "";
    }
}

// When enter is hit in the search text box, do the search
function OnSearchTextKeyPress(evt)
{
    if(evt.keyCode == 13)
    {
        PerformSearch();
        return false;
    }

    return true;
}

// Perform a keyword search
function PerformSearch()
{
    var xmlHttp = GetXmlHttpRequest(), now = new Date();

    if(xmlHttp == null)
    {
        divSearchResults.innerHTML = "<b>XML HTTP request not supported!</b>";
        return;
    }

    divSearchResults.innerHTML = "<span class=\"PaddedText\">Searching...</span>";

    // Add a unique hash to ensure it doesn't use cached results
    xmlHttp.open("GET", "SearchHelp.aspx?Keywords=" + txtSearchText.value +
        "&SortByTitle=" + (chkSortByTitle.checked ? "true" : "false") +
        "&hash=" + now.getTime(), true);

    xmlHttp.onreadystatechange = function()
    {
        if(xmlHttp.readyState == 4)
        {
            divSearchResults.innerHTML = xmlHttp.responseText;

            lastSearchNode = divSearchResults.childNodes[0].childNodes[1];

            if(lastSearchNode != null)
            {
                if(lastSearchNode.tagName != "A")
                    lastSearchNode = lastSearchNode.nextSibling;

                SelectSearchNode(lastSearchNode);
                topicContent.src = lastSearchNode.href;
            }
        }
    }

    xmlHttp.send(null)
}

// Set the style of the specified search result node to "selected"
function SelectSearchNode(node)
{
    if(lastSearchNode != null)
        lastSearchNode.className = "UnselectedNode";

    node.className = "SelectedNode";
    lastSearchNode = node;

    return true;
}

//============================================================================
// KeyWordIndex code

function ShowHideIndex(show)
{
    if(show)
    {
        PopulateIndex(currentIndexPage);

        divNavOpts.style.display = divTree.style.display = "none";
        divIndexOpts.style.display = divIndexResults.style.display = "";
    }
    else
    {
        divIndexOpts.style.display = divIndexResults.style.display = "none";
        divNavOpts.style.display = divTree.style.display = "";
    }
}

// Populate keyword index
function PopulateIndex(startIndex)
{
    var xmlHttp = GetXmlHttpRequest(), now = new Date();
    var firstNode;

    if(xmlHttp == null)
    {
        divIndexResults.innerHTML = "<b>XML HTTP request not supported!</b>";
        return;
    }

    divIndexResults.innerHTML = "<span class=\"PaddedText\">Loading " +
        "keyword index...</span>";

    // Add a unique hash to ensure it doesn't use cached results
    xmlHttp.open("GET", "LoadIndexKeywords.aspx?StartIndex=" + startIndex +
      "&hash=" + now.getTime(), true);

    xmlHttp.onreadystatechange = function()
    {
        if(xmlHttp.readyState == 4)
        {
            divIndexResults.innerHTML = xmlHttp.responseText;

            if(startIndex > 0)
            {
                firstNode = divIndexResults.childNodes[1];

                if(firstNode != null && !firstNode.innerHTML)
                    firstNode = divIndexResults.childNodes[2];
            }
            else
                firstNode = divIndexResults.childNodes[0];

            if(firstNode != null)
                lastIndexNode = firstNode.childNodes[0];

            if(lastIndexNode != null)
            {
                if(lastIndexNode.tagName != "A")
                    lastIndexNode = lastIndexNode.nextSibling;

                SelectIndexNode(lastIndexNode);
                topicContent.src = lastIndexNode.href;
            }

            currentIndexPage = startIndex;
        }
    }

    xmlHttp.send(null)
}

// Set the style of the specified keyword index node to "selected"
function SelectIndexNode(node)
{
    if(lastIndexNode != null)
        lastIndexNode.className = "UnselectedNode";

    node.className = "SelectedNode";
    lastIndexNode = node;

    return true;
}

// Changes the current page with keyword index forward or backward
function ChangeIndexPage(direction)
{
    PopulateIndex(currentIndexPage + direction);

    return false;
}
