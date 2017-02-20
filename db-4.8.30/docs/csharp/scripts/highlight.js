//=============================================================================
// System  : Color Syntax Highlighter
// File    : Highlight.js
// Author  : Eric Woodruff  (Eric@EWoodruff.us)
// Updated : 11/13/2007
// Note    : Copyright 2006, Eric Woodruff, All rights reserved
//
// This contains the script to expand and collapse the regions in the
// syntax highlighted code.
//
//=============================================================================

// Expand/collapse a region
function HighlightExpandCollapse(showId, hideId)
{
    var showSpan = document.getElementById(showId),
        hideSpan = document.getElementById(hideId);

    showSpan.style.display = "inline";
    hideSpan.style.display = "none";
}

// Copy the code if Enter or Space is hit with the image focused
function CopyColorizedCodeCheckKey(titleDiv, eventObj)
{
    if(eventObj != undefined && (eventObj.keyCode == 13 ||
      eventObj.keyCode == 32))
        CopyColorizedCode(titleDiv);
}

// Change the icon as the mouse moves in and out of the Copy Code link
// There should be an image with the same name but an "_h" suffix just
// before the extension.
function CopyCodeChangeIcon(linkSpan)
{
    var image = linkSpan.firstChild.src;
    var pos = image.lastIndexOf(".");

    if(linkSpan.className == "highlight-copycode")
    {
        linkSpan.className = "highlight-copycode_h";
        linkSpan.firstChild.src = image.substr(0, pos) + "_h" +
            image.substr(pos);
    }
    else
    {
        linkSpan.className = "highlight-copycode";
        linkSpan.firstChild.src = image.substr(0, pos - 2) + image.substr(pos);
    }
}

// Copy the code from a colorized code block to the clipboard.
function CopyColorizedCode(titleDiv)
{
    var preTag, idx, line, block, htmlLines, lines, codeText, hasLineNos,
        hasRegions, clip, trans, copyObject, clipID;
    var reLineNo = /^\s*\d{1,4}/;
    var reRegion = /^\s*\d{1,4}\+.*?\d{1,4}-/;
    var reRegionText = /^\+.*?\-/;

    // Find the <pre> tag containing the code.  It should be in the next
    // element or one of its children.
    block = titleDiv.nextSibling;

    while(block.nodeName == "#text")
        block = block.nextSibling;

    while(block.tagName != "PRE")
    {
        block = block.firstChild;

        while(block.nodeName == "#text")
            block = block.nextSibling;
    }

    if(block.innerText != undefined)
        codeText = block.innerText;
    else
        codeText = block.textContent;

    hasLineNos = block.innerHTML.indexOf("highlight-lineno");
    hasRegions = block.innerHTML.indexOf("highlight-collapsebox");
    htmlLines = block.innerHTML.split("\n");
    lines = codeText.split("\n");

    // Remove the line numbering and collapsible regions if present
    if(hasLineNos != -1 || hasRegions != -1)
    {
        codeText = "";

        for(idx = 0; idx < lines.length; idx++)
        {
            line = lines[idx];

            if(hasRegions && reRegion.test(line))
                line = line.replace(reRegion, "");
            else
            {
                line = line.replace(reLineNo, "");

                // Lines in expanded blocks have an extra space
                if(htmlLines[idx].indexOf("highlight-expanded") != -1 ||
                  htmlLines[idx].indexOf("highlight-endblock") != -1)
                    line = line.substr(1);
            }

            if(hasRegions && reRegionText.test(line))
                line = line.replace(reRegionText, "");

            codeText += line;

            // Not all browsers keep the line feed when split
            if(line[line.length - 1] != "\n")
                codeText += "\n";
        }
    }

    // IE or FireFox/Netscape?
    if(window.clipboardData)
        window.clipboardData.setData("Text", codeText);
    else
        if(window.netscape)
        {
            // Give unrestricted access to browser APIs using XPConnect
            try
            {
                netscape.security.PrivilegeManager.enablePrivilege(
                    "UniversalXPConnect");
            }
            catch(e)
            {
                alert("Universal Connect was refused, cannot copy to " +
                    "clipboard.  Go to about:config and set " +
                    "signed.applets.codebase_principal_support to true to " +
                    "enable clipboard support.");
                return;
            }

            // Creates an instance of nsIClipboard
            clip = Components.classes[
                "@mozilla.org/widget/clipboard;1"].createInstance(
                Components.interfaces.nsIClipboard);

            // Creates an instance of nsITransferable
            if(clip)
                trans = Components.classes[
                    "@mozilla.org/widget/transferable;1"].createInstance(
                    Components.interfaces.nsITransferable);

            if(!trans)
            {
                alert("Copy to Clipboard is not supported by this browser");
                return;
            }

            // Register the data flavor
            trans.addDataFlavor("text/unicode");

            // Create object to hold the data
            copyObject = new Object();

            // Creates an instance of nsISupportsString
            copyObject = Components.classes[
                "@mozilla.org/supports-string;1"].createInstance(
                Components.interfaces.nsISupportsString);

            // Assign the data to be copied
            copyObject.data = codeText;

            // Add data objects to transferable
            trans.setTransferData("text/unicode", copyObject,
                codeText.length * 2);

            clipID = Components.interfaces.nsIClipboard;

            if(!clipID)
            {
                alert("Copy to Clipboard is not supported by this browser");
                return;
            }

            // Transfer the data to the clipboard
            clip.setData(trans, null, clipID.kGlobalClipboard);
        }
        else
            alert("Copy to Clipboard is not supported by this browser");
}
