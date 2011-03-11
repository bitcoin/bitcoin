
    

  

<!DOCTYPE html>
<html>
  <head>
    <meta charset='utf-8'>
    <meta http-equiv="X-UA-Compatible" content="chrome=1">
        <title>init.cpp at f0fcb982f7f6a5184df47250b2396d92476c7d99 from TheBlueMatt/bitcoin - GitHub</title>
    <link rel="search" type="application/opensearchdescription+xml" href="/opensearch.xml" title="GitHub" />
    <link rel="fluid-icon" href="https://github.com/fluidicon.png" title="GitHub" />

    <link href="https://assets1.github.com/stylesheets/bundle_common.css?2b893794092cd44a789db64b3da95d1091187f55" media="screen" rel="stylesheet" type="text/css" />
<link href="https://assets0.github.com/stylesheets/bundle_github.css?2b893794092cd44a789db64b3da95d1091187f55" media="screen" rel="stylesheet" type="text/css" />

    <script type="text/javascript">
      if (typeof console == "undefined" || typeof console.log == "undefined")
        console = { log: function() {} }
    </script>
    <script type="text/javascript" charset="utf-8">
      var GitHub = {}
      var github_user = 'TheBlueMatt'
      
    </script>
    <script src="https://assets3.github.com/javascripts/jquery/jquery-1.4.2.min.js?2b893794092cd44a789db64b3da95d1091187f55" type="text/javascript"></script>
    <script src="https://assets3.github.com/javascripts/bundle_common.js?2b893794092cd44a789db64b3da95d1091187f55" type="text/javascript"></script>
<script src="https://assets0.github.com/javascripts/bundle_github.js?2b893794092cd44a789db64b3da95d1091187f55" type="text/javascript"></script>


        <script type="text/javascript" charset="utf-8">
      GitHub.spy({
        repo: "TheBlueMatt/bitcoin"
      })
    </script>

    
  <link href="https://github.com/TheBlueMatt/bitcoin/commits/f0fcb982f7f6a5184df47250b2396d92476c7d99.atom" rel="alternate" title="Recent Commits to bitcoin:f0fcb982f7f6a5184df47250b2396d92476c7d99" type="application/atom+xml" />

    <META NAME="ROBOTS" CONTENT="NOINDEX, FOLLOW">    <meta name="description" content="Bitcoin integration/staging tree" />
    <script type="text/javascript">
      GitHub.nameWithOwner = GitHub.nameWithOwner || "TheBlueMatt/bitcoin";
      GitHub.currentRef = '';
      GitHub.commitSHA = "f0fcb982f7f6a5184df47250b2396d92476c7d99";
      GitHub.currentPath = 'init.cpp';
      GitHub.masterBranch = "master";

      
    </script>
  

        <script type="text/javascript">
      var _gaq = _gaq || [];
      _gaq.push(['_setAccount', 'UA-3769691-2']);
      _gaq.push(['_setDomainName', 'none']);
      _gaq.push(['_trackPageview']);
      (function() {
        var ga = document.createElement('script');
        ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
        ga.setAttribute('async', 'true');
        document.documentElement.firstChild.appendChild(ga);
      })();
    </script>

    
  </head>

  

  <body class="logged_in page-blob">
    

    

    

    

    

    <div class="subnavd" id="main">
      <div id="header" class="true">
        
          <a class="logo boring" href="https://github.com/">
            <img alt="github" class="default" src="https://assets0.github.com/images/modules/header/logov3.png?2b893794092cd44a789db64b3da95d1091187f55" />
            <!--[if (gt IE 8)|!(IE)]><!-->
            <img alt="github" class="hover" src="https://assets2.github.com/images/modules/header/logov3-hover.png?2b893794092cd44a789db64b3da95d1091187f55" />
            <!--<![endif]-->
          </a>
        
        
          





  
    <div class="userbox">
      <div class="avatarname">
        <a href="https://github.com/TheBlueMatt"><img src="https://secure.gravatar.com/avatar/c0d69eae553e6965038ba73dde6dead7?s=140&d=https://assets.github.com%2Fimages%2Fgravatars%2Fgravatar-140.png" alt="" width="20" height="20"  /></a>
        <a href="https://github.com/TheBlueMatt" class="name">TheBlueMatt</a>

        
        
      </div>
      <ul class="usernav">
        <li><a href="https://github.com/">Dashboard</a></li>
        <li>
          
          <a href="https://github.com/inbox">Inbox</a>
          <a href="https://github.com/inbox" class="unread_count ">0</a>
        </li>
        <li><a href="https://github.com/account">Account Settings</a></li>
                <li><a href="/logout">Log Out</a></li>
      </ul>
    </div><!-- /.userbox -->
  


        
        <div class="topsearch">
  
    <form action="/search" id="top_search_form" method="get">
      <a href="/search" class="advanced-search tooltipped downwards" title="Advanced Search">Advanced Search</a>
      <input type="search" class="search my_repos_autocompleter" name="q" results="5" placeholder="Search&hellip;" /> <input type="submit" value="Search" class="button" />
      <input type="hidden" name="type" value="Everything" />
      <input type="hidden" name="repo" value="" />
      <input type="hidden" name="langOverride" value="" />
      <input type="hidden" name="start_value" value="1" />
    </form>
    <ul class="nav">
      <li><a href="/explore">Explore GitHub</a></li>
      <li><a href="https://gist.github.com">Gist</a></li>
      <li><a href="/blog">Blog</a></li>
      <li><a href="http://help.github.com">Help</a></li>
    </ul>
  
</div>

      </div>

      
      
        
    <div class="site">
      <div class="pagehead repohead vis-public fork  ">

      

      <div class="title-actions-bar">
        <h1>
          <a href="/TheBlueMatt">TheBlueMatt</a> / <strong><a href="https://github.com/TheBlueMatt/bitcoin">bitcoin</a></strong>
          
            <span class="fork-flag">
              
              <span class="text">forked from <a href="/bitcoin/bitcoin">bitcoin/bitcoin</a></span>
            </span>
          
          
        </h1>

        
    <ul class="actions">
      

      
        <li class="for-owner" style="display:none"><a href="https://github.com/TheBlueMatt/bitcoin/admin" class="minibutton btn-admin "><span><span class="icon"></span>Admin</span></a></li>
        <li>
          <a href="/TheBlueMatt/bitcoin/toggle_watch" class="minibutton btn-watch " id="watch_button" onclick="var f = document.createElement('form'); f.style.display = 'none'; this.parentNode.appendChild(f); f.method = 'POST'; f.action = this.href;var s = document.createElement('input'); s.setAttribute('type', 'hidden'); s.setAttribute('name', 'authenticity_token'); s.setAttribute('value', 'cf1c0b8959733617e26844d243ba9e11c32dfb89'); f.appendChild(s);f.submit();return false;" style="display:none"><span><span class="icon"></span>Watch</span></a>
          <a href="/TheBlueMatt/bitcoin/toggle_watch" class="minibutton btn-watch " id="unwatch_button" onclick="var f = document.createElement('form'); f.style.display = 'none'; this.parentNode.appendChild(f); f.method = 'POST'; f.action = this.href;var s = document.createElement('input'); s.setAttribute('type', 'hidden'); s.setAttribute('name', 'authenticity_token'); s.setAttribute('value', 'cf1c0b8959733617e26844d243ba9e11c32dfb89'); f.appendChild(s);f.submit();return false;" style="display:none"><span><span class="icon"></span>Unwatch</span></a>
        </li>
        
          
            <li class="for-notforked" style="display:none"><a href="/TheBlueMatt/bitcoin/fork" class="minibutton btn-fork " id="fork_button" onclick="var f = document.createElement('form'); f.style.display = 'none'; this.parentNode.appendChild(f); f.method = 'POST'; f.action = this.href;var s = document.createElement('input'); s.setAttribute('type', 'hidden'); s.setAttribute('name', 'authenticity_token'); s.setAttribute('value', 'cf1c0b8959733617e26844d243ba9e11c32dfb89'); f.appendChild(s);f.submit();return false;"><span><span class="icon"></span>Fork</span></a></li>
            <li class="for-hasfork" style="display:none"><a href="#" class="minibutton btn-fork " id="your_fork_button"><span><span class="icon"></span>Your Fork</span></a></li>
          

          <li id='pull_request_item' class='nspr' style='display:none'><a href="/TheBlueMatt/bitcoin/pull/new/f0fcb982f7f6a5184df47250b2396d92476c7d99" class="minibutton btn-pull-request "><span><span class="icon"></span>Pull Request</span></a></li>
        
      
      
      <li class="repostats">
        <ul class="repo-stats">
          <li class="watchers"><a href="/TheBlueMatt/bitcoin/watchers" title="Watchers" class="tooltipped downwards">1</a></li>
          <li class="forks"><a href="/TheBlueMatt/bitcoin/network" title="Forks" class="tooltipped downwards">48</a></li>
        </ul>
      </li>
    </ul>

      </div>

        
  <ul class="tabs">
    <li><a href="https://github.com/TheBlueMatt/bitcoin/tree/" class="selected" highlight="repo_source">Source</a></li>
    <li><a href="https://github.com/TheBlueMatt/bitcoin/commits/" highlight="repo_commits">Commits</a></li>
    <li><a href="/TheBlueMatt/bitcoin/network" highlight="repo_network">Network</a></li>
    <li><a href="/TheBlueMatt/bitcoin/pulls" highlight="repo_pulls">Pull Requests (0)</a></li>

    
      <li><a href="/TheBlueMatt/bitcoin/forkqueue" highlight="repo_fork_queue">Fork Queue</a></li>
    

    

                <li><a href="/TheBlueMatt/bitcoin/wiki" highlight="repo_wiki">Wiki (0)</a></li>
        
    <li><a href="/TheBlueMatt/bitcoin/graphs" highlight="repo_graphs">Graphs</a></li>

    <li class="contextswitch nochoices">
      <span class="toggle leftwards tooltipped" title="f0fcb982f7f6a5184df47250b2396d92476c7d99">
        <em>Tree:</em>
        <code>f0fcb98</code>
      </span>
    </li>
  </ul>

  <div style="display:none" id="pl-description"><p><em class="placeholder">click here to add a description</em></p></div>
  <div style="display:none" id="pl-homepage"><p><em class="placeholder">click here to add a homepage</em></p></div>

  <div class="subnav-bar">
  
  <ul>
    <li>
      <a href="#" class="dropdown">Switch Branches (6)</a>
      <ul>
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/blockheaders/init.cpp" action="show">blockheaders</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/log-timestamp/init.cpp" action="show">log-timestamp</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/master/init.cpp" action="show">master</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/portoption/init.cpp" action="show">portoption</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/setaccountfix/init.cpp" action="show">setaccountfix</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/upnp/init.cpp" action="show">upnp</a></li>
          
        
      </ul>
    </li>
    <li>
      <a href="#" class="dropdown ">Switch Tags (3)</a>
              <ul>
                      
              <li><a href="/TheBlueMatt/bitcoin/blob/v0.3.20.2/init.cpp">v0.3.20.2</a></li>
            
                      
              <li><a href="/TheBlueMatt/bitcoin/blob/v0.3.20/init.cpp">v0.3.20</a></li>
            
                      
              <li><a href="/TheBlueMatt/bitcoin/blob/v0.3.19/init.cpp">v0.3.19</a></li>
            
                  </ul>
      
    </li>
    <li>
    
    <a href="/TheBlueMatt/bitcoin/branches/master" class="manage">Branch List</a>
    
    </li>
  </ul>
</div>

  
  
  
  
  
  



        
    <div class="frame frame-center tree-finder" style="display: none">
      <div class="breadcrumb">
        <b><a href="/TheBlueMatt/bitcoin">bitcoin</a></b> /
        <input class="tree-finder-input" type="text" name="query" autocomplete="off" spellcheck="false">
      </div>

      
        <div class="octotip">
          <p>
            <a href="/TheBlueMatt/bitcoin/dismiss-tree-finder-help" class="dismiss js-dismiss-tree-list-help" title="Hide this notice forever">Dismiss</a>
            <strong>Octotip:</strong> You've activated the <em>file finder</em> by pressing <span class="kbd">t</span>
            Start typing to filter the file list. Use <span class="kbd badmono">↑</span> and <span class="kbd badmono">↓</span> to navigate,
            <span class="kbd">enter</span> to view files.
          </p>
        </div>
      

      <table class="tree-browser" cellpadding="0" cellspacing="0">
        <tr class="js-header"><th>&nbsp;</th><th>name</th></tr>
        <tr class="js-no-results no-results" style="display: none">
          <th colspan="2">No matching files</th>
        </tr>
        <tbody class="js-results-list">
        </tbody>
      </table>
    </div>

    <div id="jump-to-line" style="display:none">
      <h2>Jump to Line</h2>
      <form>
        <input class="textfield" type="text">
        <div class="full-button">
          <button type="submit" class="classy">
            <span>Go</span>
          </button>
        </div>
      </form>
    </div>

    <div id="repo_details" class="metabox clearfix">
      <div id="repo_details_loader" class="metabox-loader" style="display:none">Sending Request&hellip;</div>

        <a href="/TheBlueMatt/bitcoin/downloads" class="download-source" id="download_button" title="Download source, tagged packages and binaries."><span class="icon"></span>Downloads</a>

      <div id="repository_desc_wrapper">
      <div id="repository_description" rel="repository_description_edit">
        
          <p>Bitcoin integration/staging tree
            <span id="read_more" style="display:none">&mdash; <a href="#readme">Read more</a></span>
          </p>
        
      </div>

      <div id="repository_description_edit" style="display:none;" class="inline-edit">
        <form action="/TheBlueMatt/bitcoin/admin/update" method="post"><div style="margin:0;padding:0"><input name="authenticity_token" type="hidden" value="cf1c0b8959733617e26844d243ba9e11c32dfb89" /></div>
          <input type="hidden" name="field" value="repository_description">
          <input type="text" class="textfield" name="value" value="Bitcoin integration/staging tree">
          <div class="form-actions">
            <button class="minibutton"><span>Save</span></button> &nbsp; <a href="#" class="cancel">Cancel</a>
          </div>
        </form>
      </div>

      
      <div class="repository-homepage" id="repository_homepage" rel="repository_homepage_edit">
        <p><a href="http://www.bitcoin.org" rel="nofollow">http://www.bitcoin.org</a></p>
      </div>

      <div id="repository_homepage_edit" style="display:none;" class="inline-edit">
        <form action="/TheBlueMatt/bitcoin/admin/update" method="post"><div style="margin:0;padding:0"><input name="authenticity_token" type="hidden" value="cf1c0b8959733617e26844d243ba9e11c32dfb89" /></div>
          <input type="hidden" name="field" value="repository_homepage">
          <input type="text" class="textfield" name="value" value="http://www.bitcoin.org">
          <div class="form-actions">
            <button class="minibutton"><span>Save</span></button> &nbsp; <a href="#" class="cancel">Cancel</a>
          </div>
        </form>
      </div>
      </div>
      <div class="rule "></div>
            <div id="url_box" class="url-box">
        <ul class="clone-urls">
          
            
              <li id="private_clone_url"><a href="git@github.com:TheBlueMatt/bitcoin.git" data-permissions="Read+Write">SSH</a></li>
            
            <li id="http_clone_url"><a href="https://TheBlueMatt@github.com/TheBlueMatt/bitcoin.git" data-permissions="Read+Write">HTTP</a></li>
            <li id="public_clone_url"><a href="git://github.com/TheBlueMatt/bitcoin.git" data-permissions="Read-Only">Git Read-Only</a></li>
          
          
        </ul>
        <input type="text" spellcheck="false" id="url_field" class="url-field" />
              <span style="display:none" id="url_box_clippy"></span>
      <span id="clippy_tooltip_url_box_clippy" class="clippy-tooltip tooltipped" title="copy to clipboard">
      <object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"
              width="14"
              height="14"
              class="clippy"
              id="clippy" >
      <param name="movie" value="https://assets2.github.com/flash/clippy.swf?v5"/>
      <param name="allowScriptAccess" value="always" />
      <param name="quality" value="high" />
      <param name="scale" value="noscale" />
      <param NAME="FlashVars" value="id=url_box_clippy&amp;copied=&amp;copyto=">
      <param name="bgcolor" value="#FFFFFF">
      <param name="wmode" value="opaque">
      <embed src="https://assets2.github.com/flash/clippy.swf?v5"
             width="14"
             height="14"
             name="clippy"
             quality="high"
             allowScriptAccess="always"
             type="application/x-shockwave-flash"
             pluginspage="http://www.macromedia.com/go/getflashplayer"
             FlashVars="id=url_box_clippy&amp;copied=&amp;copyto="
             bgcolor="#FFFFFF"
             wmode="opaque"
      />
      </object>
      </span>

        <p id="url_description">This URL has <strong>Read+Write</strong> access</p>
      </div>
    </div>


        

      </div><!-- /.pagehead -->

      

  





<script type="text/javascript">
  GitHub.downloadRepo = '/TheBlueMatt/bitcoin/archives/f0fcb982f7f6a5184df47250b2396d92476c7d99'
  GitHub.revType = "SHA1"

  GitHub.controllerName = "blob"
  GitHub.actionName     = "show"
  GitHub.currentAction  = "blob#show"

  
    GitHub.hasWriteAccess = true
    GitHub.hasAdminAccess = true
    GitHub.watchingRepo = true
    GitHub.ignoredRepo = false
    GitHub.hasForkOfRepo = null
    GitHub.hasForked = false
  

  
</script>






<div class="flash-messages"></div>


  <div id="commit">
    <div class="group">
        
  <div class="envelope commit">
    <div class="human">
      
        <div class="message"><pre><a href="/TheBlueMatt/bitcoin/commit/f0fcb982f7f6a5184df47250b2396d92476c7d99">Be a bit more clear in the build instructions for Unix.</a> </pre></div>
      

      <div class="actor">
        <div class="gravatar">
          
          <img src="https://secure.gravatar.com/avatar/a9d8a2fc99b09023085c134fc4619f2d?s=140&d=https://assets.github.com%2Fimages%2Fgravatars%2Fgravatar-140.png" alt="" width="30" height="30"  />
        </div>
        <div class="name">Matt Corallo <span>(author)</span></div>
        <div class="date">
          <abbr class="relatize" title="2011-03-11 13:57:50">Fri Mar 11 13:57:50 -0800 2011</abbr>
        </div>
      </div>

      

    </div>
    <div class="machine">
      <span>c</span>ommit&nbsp;&nbsp;<a href="/TheBlueMatt/bitcoin/commit/f0fcb982f7f6a5184df47250b2396d92476c7d99" hotkey="c">f0fcb982f7f6a5184df4</a><br />
      <span>t</span>ree&nbsp;&nbsp;&nbsp;&nbsp;<a href="/TheBlueMatt/bitcoin/tree/f0fcb982f7f6a5184df47250b2396d92476c7d99/build-unix.txt" hotkey="t">306da4a709069ad6f4a0</a><br />
      
        <span>p</span>arent&nbsp;
        
        <a href="/TheBlueMatt/bitcoin/commit/fd7e34f46f6c4e3a065e3604ae43e9c0425aad71" hotkey="p">fd7e34f46f6c4e3a065e</a>
      

    </div>
  </div>

    </div>
  </div>



  <div id="slider">

  

    <div class="breadcrumb" data-path="init.cpp/">
      <b><a href="/TheBlueMatt/bitcoin/tree/a2f7dbe95df0d9dde94f54cfb4875db700280c0f">bitcoin</a></b> / init.cpp       <span style="display:none" id="clippy_4579">init.cpp</span>
      
      <object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"
              width="110"
              height="14"
              class="clippy"
              id="clippy" >
      <param name="movie" value="https://assets2.github.com/flash/clippy.swf?v5"/>
      <param name="allowScriptAccess" value="always" />
      <param name="quality" value="high" />
      <param name="scale" value="noscale" />
      <param NAME="FlashVars" value="id=clippy_4579&amp;copied=copied!&amp;copyto=copy to clipboard">
      <param name="bgcolor" value="#FFFFFF">
      <param name="wmode" value="opaque">
      <embed src="https://assets2.github.com/flash/clippy.swf?v5"
             width="110"
             height="14"
             name="clippy"
             quality="high"
             allowScriptAccess="always"
             type="application/x-shockwave-flash"
             pluginspage="http://www.macromedia.com/go/getflashplayer"
             FlashVars="id=clippy_4579&amp;copied=copied!&amp;copyto=copy to clipboard"
             bgcolor="#FFFFFF"
             wmode="opaque"
      />
      </object>
      

    </div>

    <div class="frames">
      <div class="frame frame-center" data-path="init.cpp/">
        
          <ul class="big-actions">
            
            <li><a class="file-edit-link minibutton" href="/TheBlueMatt/bitcoin/file-edit/__current_ref__/init.cpp"><span>Edit this file</span></a></li>
          </ul>
        

        <div id="files">
          <div class="file">
            <div class="meta">
              <div class="info">
                <span class="icon"><img alt="Txt" height="16" src="https://assets0.github.com/images/icons/txt.png?2b893794092cd44a789db64b3da95d1091187f55" width="16" /></span>
                <span class="mode" title="File Mode">100644</span>
                
                  <span>463 lines (398 sloc)</span>
                
                <span>14.729 kb</span>
              </div>
              <ul class="actions">
                <li><a href="/TheBlueMatt/bitcoin/raw/a2f7dbe95df0d9dde94f54cfb4875db700280c0f/init.cpp" id="raw-url">raw</a></li>
                
                  <li><a href="/TheBlueMatt/bitcoin/blame/a2f7dbe95df0d9dde94f54cfb4875db700280c0f/init.cpp">blame</a></li>
                
                <li><a href="/TheBlueMatt/bitcoin/commits/a2f7dbe95df0d9dde94f54cfb4875db700280c0f/init.cpp">history</a></li>
              </ul>
            </div>
            
  <div class="data type-cpp">
    
      <table cellpadding="0" cellspacing="0">
        <tr>
          <td>
            <pre class="line_numbers"><span id="L1" rel="#L1">1</span>
<span id="L2" rel="#L2">2</span>
<span id="L3" rel="#L3">3</span>
<span id="L4" rel="#L4">4</span>
<span id="L5" rel="#L5">5</span>
<span id="L6" rel="#L6">6</span>
<span id="L7" rel="#L7">7</span>
<span id="L8" rel="#L8">8</span>
<span id="L9" rel="#L9">9</span>
<span id="L10" rel="#L10">10</span>
<span id="L11" rel="#L11">11</span>
<span id="L12" rel="#L12">12</span>
<span id="L13" rel="#L13">13</span>
<span id="L14" rel="#L14">14</span>
<span id="L15" rel="#L15">15</span>
<span id="L16" rel="#L16">16</span>
<span id="L17" rel="#L17">17</span>
<span id="L18" rel="#L18">18</span>
<span id="L19" rel="#L19">19</span>
<span id="L20" rel="#L20">20</span>
<span id="L21" rel="#L21">21</span>
<span id="L22" rel="#L22">22</span>
<span id="L23" rel="#L23">23</span>
<span id="L24" rel="#L24">24</span>
<span id="L25" rel="#L25">25</span>
<span id="L26" rel="#L26">26</span>
<span id="L27" rel="#L27">27</span>
<span id="L28" rel="#L28">28</span>
<span id="L29" rel="#L29">29</span>
<span id="L30" rel="#L30">30</span>
<span id="L31" rel="#L31">31</span>
<span id="L32" rel="#L32">32</span>
<span id="L33" rel="#L33">33</span>
<span id="L34" rel="#L34">34</span>
<span id="L35" rel="#L35">35</span>
<span id="L36" rel="#L36">36</span>
<span id="L37" rel="#L37">37</span>
<span id="L38" rel="#L38">38</span>
<span id="L39" rel="#L39">39</span>
<span id="L40" rel="#L40">40</span>
<span id="L41" rel="#L41">41</span>
<span id="L42" rel="#L42">42</span>
<span id="L43" rel="#L43">43</span>
<span id="L44" rel="#L44">44</span>
<span id="L45" rel="#L45">45</span>
<span id="L46" rel="#L46">46</span>
<span id="L47" rel="#L47">47</span>
<span id="L48" rel="#L48">48</span>
<span id="L49" rel="#L49">49</span>
<span id="L50" rel="#L50">50</span>
<span id="L51" rel="#L51">51</span>
<span id="L52" rel="#L52">52</span>
<span id="L53" rel="#L53">53</span>
<span id="L54" rel="#L54">54</span>
<span id="L55" rel="#L55">55</span>
<span id="L56" rel="#L56">56</span>
<span id="L57" rel="#L57">57</span>
<span id="L58" rel="#L58">58</span>
<span id="L59" rel="#L59">59</span>
<span id="L60" rel="#L60">60</span>
<span id="L61" rel="#L61">61</span>
<span id="L62" rel="#L62">62</span>
<span id="L63" rel="#L63">63</span>
<span id="L64" rel="#L64">64</span>
<span id="L65" rel="#L65">65</span>
<span id="L66" rel="#L66">66</span>
<span id="L67" rel="#L67">67</span>
<span id="L68" rel="#L68">68</span>
<span id="L69" rel="#L69">69</span>
<span id="L70" rel="#L70">70</span>
<span id="L71" rel="#L71">71</span>
<span id="L72" rel="#L72">72</span>
<span id="L73" rel="#L73">73</span>
<span id="L74" rel="#L74">74</span>
<span id="L75" rel="#L75">75</span>
<span id="L76" rel="#L76">76</span>
<span id="L77" rel="#L77">77</span>
<span id="L78" rel="#L78">78</span>
<span id="L79" rel="#L79">79</span>
<span id="L80" rel="#L80">80</span>
<span id="L81" rel="#L81">81</span>
<span id="L82" rel="#L82">82</span>
<span id="L83" rel="#L83">83</span>
<span id="L84" rel="#L84">84</span>
<span id="L85" rel="#L85">85</span>
<span id="L86" rel="#L86">86</span>
<span id="L87" rel="#L87">87</span>
<span id="L88" rel="#L88">88</span>
<span id="L89" rel="#L89">89</span>
<span id="L90" rel="#L90">90</span>
<span id="L91" rel="#L91">91</span>
<span id="L92" rel="#L92">92</span>
<span id="L93" rel="#L93">93</span>
<span id="L94" rel="#L94">94</span>
<span id="L95" rel="#L95">95</span>
<span id="L96" rel="#L96">96</span>
<span id="L97" rel="#L97">97</span>
<span id="L98" rel="#L98">98</span>
<span id="L99" rel="#L99">99</span>
<span id="L100" rel="#L100">100</span>
<span id="L101" rel="#L101">101</span>
<span id="L102" rel="#L102">102</span>
<span id="L103" rel="#L103">103</span>
<span id="L104" rel="#L104">104</span>
<span id="L105" rel="#L105">105</span>
<span id="L106" rel="#L106">106</span>
<span id="L107" rel="#L107">107</span>
<span id="L108" rel="#L108">108</span>
<span id="L109" rel="#L109">109</span>
<span id="L110" rel="#L110">110</span>
<span id="L111" rel="#L111">111</span>
<span id="L112" rel="#L112">112</span>
<span id="L113" rel="#L113">113</span>
<span id="L114" rel="#L114">114</span>
<span id="L115" rel="#L115">115</span>
<span id="L116" rel="#L116">116</span>
<span id="L117" rel="#L117">117</span>
<span id="L118" rel="#L118">118</span>
<span id="L119" rel="#L119">119</span>
<span id="L120" rel="#L120">120</span>
<span id="L121" rel="#L121">121</span>
<span id="L122" rel="#L122">122</span>
<span id="L123" rel="#L123">123</span>
<span id="L124" rel="#L124">124</span>
<span id="L125" rel="#L125">125</span>
<span id="L126" rel="#L126">126</span>
<span id="L127" rel="#L127">127</span>
<span id="L128" rel="#L128">128</span>
<span id="L129" rel="#L129">129</span>
<span id="L130" rel="#L130">130</span>
<span id="L131" rel="#L131">131</span>
<span id="L132" rel="#L132">132</span>
<span id="L133" rel="#L133">133</span>
<span id="L134" rel="#L134">134</span>
<span id="L135" rel="#L135">135</span>
<span id="L136" rel="#L136">136</span>
<span id="L137" rel="#L137">137</span>
<span id="L138" rel="#L138">138</span>
<span id="L139" rel="#L139">139</span>
<span id="L140" rel="#L140">140</span>
<span id="L141" rel="#L141">141</span>
<span id="L142" rel="#L142">142</span>
<span id="L143" rel="#L143">143</span>
<span id="L144" rel="#L144">144</span>
<span id="L145" rel="#L145">145</span>
<span id="L146" rel="#L146">146</span>
<span id="L147" rel="#L147">147</span>
<span id="L148" rel="#L148">148</span>
<span id="L149" rel="#L149">149</span>
<span id="L150" rel="#L150">150</span>
<span id="L151" rel="#L151">151</span>
<span id="L152" rel="#L152">152</span>
<span id="L153" rel="#L153">153</span>
<span id="L154" rel="#L154">154</span>
<span id="L155" rel="#L155">155</span>
<span id="L156" rel="#L156">156</span>
<span id="L157" rel="#L157">157</span>
<span id="L158" rel="#L158">158</span>
<span id="L159" rel="#L159">159</span>
<span id="L160" rel="#L160">160</span>
<span id="L161" rel="#L161">161</span>
<span id="L162" rel="#L162">162</span>
<span id="L163" rel="#L163">163</span>
<span id="L164" rel="#L164">164</span>
<span id="L165" rel="#L165">165</span>
<span id="L166" rel="#L166">166</span>
<span id="L167" rel="#L167">167</span>
<span id="L168" rel="#L168">168</span>
<span id="L169" rel="#L169">169</span>
<span id="L170" rel="#L170">170</span>
<span id="L171" rel="#L171">171</span>
<span id="L172" rel="#L172">172</span>
<span id="L173" rel="#L173">173</span>
<span id="L174" rel="#L174">174</span>
<span id="L175" rel="#L175">175</span>
<span id="L176" rel="#L176">176</span>
<span id="L177" rel="#L177">177</span>
<span id="L178" rel="#L178">178</span>
<span id="L179" rel="#L179">179</span>
<span id="L180" rel="#L180">180</span>
<span id="L181" rel="#L181">181</span>
<span id="L182" rel="#L182">182</span>
<span id="L183" rel="#L183">183</span>
<span id="L184" rel="#L184">184</span>
<span id="L185" rel="#L185">185</span>
<span id="L186" rel="#L186">186</span>
<span id="L187" rel="#L187">187</span>
<span id="L188" rel="#L188">188</span>
<span id="L189" rel="#L189">189</span>
<span id="L190" rel="#L190">190</span>
<span id="L191" rel="#L191">191</span>
<span id="L192" rel="#L192">192</span>
<span id="L193" rel="#L193">193</span>
<span id="L194" rel="#L194">194</span>
<span id="L195" rel="#L195">195</span>
<span id="L196" rel="#L196">196</span>
<span id="L197" rel="#L197">197</span>
<span id="L198" rel="#L198">198</span>
<span id="L199" rel="#L199">199</span>
<span id="L200" rel="#L200">200</span>
<span id="L201" rel="#L201">201</span>
<span id="L202" rel="#L202">202</span>
<span id="L203" rel="#L203">203</span>
<span id="L204" rel="#L204">204</span>
<span id="L205" rel="#L205">205</span>
<span id="L206" rel="#L206">206</span>
<span id="L207" rel="#L207">207</span>
<span id="L208" rel="#L208">208</span>
<span id="L209" rel="#L209">209</span>
<span id="L210" rel="#L210">210</span>
<span id="L211" rel="#L211">211</span>
<span id="L212" rel="#L212">212</span>
<span id="L213" rel="#L213">213</span>
<span id="L214" rel="#L214">214</span>
<span id="L215" rel="#L215">215</span>
<span id="L216" rel="#L216">216</span>
<span id="L217" rel="#L217">217</span>
<span id="L218" rel="#L218">218</span>
<span id="L219" rel="#L219">219</span>
<span id="L220" rel="#L220">220</span>
<span id="L221" rel="#L221">221</span>
<span id="L222" rel="#L222">222</span>
<span id="L223" rel="#L223">223</span>
<span id="L224" rel="#L224">224</span>
<span id="L225" rel="#L225">225</span>
<span id="L226" rel="#L226">226</span>
<span id="L227" rel="#L227">227</span>
<span id="L228" rel="#L228">228</span>
<span id="L229" rel="#L229">229</span>
<span id="L230" rel="#L230">230</span>
<span id="L231" rel="#L231">231</span>
<span id="L232" rel="#L232">232</span>
<span id="L233" rel="#L233">233</span>
<span id="L234" rel="#L234">234</span>
<span id="L235" rel="#L235">235</span>
<span id="L236" rel="#L236">236</span>
<span id="L237" rel="#L237">237</span>
<span id="L238" rel="#L238">238</span>
<span id="L239" rel="#L239">239</span>
<span id="L240" rel="#L240">240</span>
<span id="L241" rel="#L241">241</span>
<span id="L242" rel="#L242">242</span>
<span id="L243" rel="#L243">243</span>
<span id="L244" rel="#L244">244</span>
<span id="L245" rel="#L245">245</span>
<span id="L246" rel="#L246">246</span>
<span id="L247" rel="#L247">247</span>
<span id="L248" rel="#L248">248</span>
<span id="L249" rel="#L249">249</span>
<span id="L250" rel="#L250">250</span>
<span id="L251" rel="#L251">251</span>
<span id="L252" rel="#L252">252</span>
<span id="L253" rel="#L253">253</span>
<span id="L254" rel="#L254">254</span>
<span id="L255" rel="#L255">255</span>
<span id="L256" rel="#L256">256</span>
<span id="L257" rel="#L257">257</span>
<span id="L258" rel="#L258">258</span>
<span id="L259" rel="#L259">259</span>
<span id="L260" rel="#L260">260</span>
<span id="L261" rel="#L261">261</span>
<span id="L262" rel="#L262">262</span>
<span id="L263" rel="#L263">263</span>
<span id="L264" rel="#L264">264</span>
<span id="L265" rel="#L265">265</span>
<span id="L266" rel="#L266">266</span>
<span id="L267" rel="#L267">267</span>
<span id="L268" rel="#L268">268</span>
<span id="L269" rel="#L269">269</span>
<span id="L270" rel="#L270">270</span>
<span id="L271" rel="#L271">271</span>
<span id="L272" rel="#L272">272</span>
<span id="L273" rel="#L273">273</span>
<span id="L274" rel="#L274">274</span>
<span id="L275" rel="#L275">275</span>
<span id="L276" rel="#L276">276</span>
<span id="L277" rel="#L277">277</span>
<span id="L278" rel="#L278">278</span>
<span id="L279" rel="#L279">279</span>
<span id="L280" rel="#L280">280</span>
<span id="L281" rel="#L281">281</span>
<span id="L282" rel="#L282">282</span>
<span id="L283" rel="#L283">283</span>
<span id="L284" rel="#L284">284</span>
<span id="L285" rel="#L285">285</span>
<span id="L286" rel="#L286">286</span>
<span id="L287" rel="#L287">287</span>
<span id="L288" rel="#L288">288</span>
<span id="L289" rel="#L289">289</span>
<span id="L290" rel="#L290">290</span>
<span id="L291" rel="#L291">291</span>
<span id="L292" rel="#L292">292</span>
<span id="L293" rel="#L293">293</span>
<span id="L294" rel="#L294">294</span>
<span id="L295" rel="#L295">295</span>
<span id="L296" rel="#L296">296</span>
<span id="L297" rel="#L297">297</span>
<span id="L298" rel="#L298">298</span>
<span id="L299" rel="#L299">299</span>
<span id="L300" rel="#L300">300</span>
<span id="L301" rel="#L301">301</span>
<span id="L302" rel="#L302">302</span>
<span id="L303" rel="#L303">303</span>
<span id="L304" rel="#L304">304</span>
<span id="L305" rel="#L305">305</span>
<span id="L306" rel="#L306">306</span>
<span id="L307" rel="#L307">307</span>
<span id="L308" rel="#L308">308</span>
<span id="L309" rel="#L309">309</span>
<span id="L310" rel="#L310">310</span>
<span id="L311" rel="#L311">311</span>
<span id="L312" rel="#L312">312</span>
<span id="L313" rel="#L313">313</span>
<span id="L314" rel="#L314">314</span>
<span id="L315" rel="#L315">315</span>
<span id="L316" rel="#L316">316</span>
<span id="L317" rel="#L317">317</span>
<span id="L318" rel="#L318">318</span>
<span id="L319" rel="#L319">319</span>
<span id="L320" rel="#L320">320</span>
<span id="L321" rel="#L321">321</span>
<span id="L322" rel="#L322">322</span>
<span id="L323" rel="#L323">323</span>
<span id="L324" rel="#L324">324</span>
<span id="L325" rel="#L325">325</span>
<span id="L326" rel="#L326">326</span>
<span id="L327" rel="#L327">327</span>
<span id="L328" rel="#L328">328</span>
<span id="L329" rel="#L329">329</span>
<span id="L330" rel="#L330">330</span>
<span id="L331" rel="#L331">331</span>
<span id="L332" rel="#L332">332</span>
<span id="L333" rel="#L333">333</span>
<span id="L334" rel="#L334">334</span>
<span id="L335" rel="#L335">335</span>
<span id="L336" rel="#L336">336</span>
<span id="L337" rel="#L337">337</span>
<span id="L338" rel="#L338">338</span>
<span id="L339" rel="#L339">339</span>
<span id="L340" rel="#L340">340</span>
<span id="L341" rel="#L341">341</span>
<span id="L342" rel="#L342">342</span>
<span id="L343" rel="#L343">343</span>
<span id="L344" rel="#L344">344</span>
<span id="L345" rel="#L345">345</span>
<span id="L346" rel="#L346">346</span>
<span id="L347" rel="#L347">347</span>
<span id="L348" rel="#L348">348</span>
<span id="L349" rel="#L349">349</span>
<span id="L350" rel="#L350">350</span>
<span id="L351" rel="#L351">351</span>
<span id="L352" rel="#L352">352</span>
<span id="L353" rel="#L353">353</span>
<span id="L354" rel="#L354">354</span>
<span id="L355" rel="#L355">355</span>
<span id="L356" rel="#L356">356</span>
<span id="L357" rel="#L357">357</span>
<span id="L358" rel="#L358">358</span>
<span id="L359" rel="#L359">359</span>
<span id="L360" rel="#L360">360</span>
<span id="L361" rel="#L361">361</span>
<span id="L362" rel="#L362">362</span>
<span id="L363" rel="#L363">363</span>
<span id="L364" rel="#L364">364</span>
<span id="L365" rel="#L365">365</span>
<span id="L366" rel="#L366">366</span>
<span id="L367" rel="#L367">367</span>
<span id="L368" rel="#L368">368</span>
<span id="L369" rel="#L369">369</span>
<span id="L370" rel="#L370">370</span>
<span id="L371" rel="#L371">371</span>
<span id="L372" rel="#L372">372</span>
<span id="L373" rel="#L373">373</span>
<span id="L374" rel="#L374">374</span>
<span id="L375" rel="#L375">375</span>
<span id="L376" rel="#L376">376</span>
<span id="L377" rel="#L377">377</span>
<span id="L378" rel="#L378">378</span>
<span id="L379" rel="#L379">379</span>
<span id="L380" rel="#L380">380</span>
<span id="L381" rel="#L381">381</span>
<span id="L382" rel="#L382">382</span>
<span id="L383" rel="#L383">383</span>
<span id="L384" rel="#L384">384</span>
<span id="L385" rel="#L385">385</span>
<span id="L386" rel="#L386">386</span>
<span id="L387" rel="#L387">387</span>
<span id="L388" rel="#L388">388</span>
<span id="L389" rel="#L389">389</span>
<span id="L390" rel="#L390">390</span>
<span id="L391" rel="#L391">391</span>
<span id="L392" rel="#L392">392</span>
<span id="L393" rel="#L393">393</span>
<span id="L394" rel="#L394">394</span>
<span id="L395" rel="#L395">395</span>
<span id="L396" rel="#L396">396</span>
<span id="L397" rel="#L397">397</span>
<span id="L398" rel="#L398">398</span>
<span id="L399" rel="#L399">399</span>
<span id="L400" rel="#L400">400</span>
<span id="L401" rel="#L401">401</span>
<span id="L402" rel="#L402">402</span>
<span id="L403" rel="#L403">403</span>
<span id="L404" rel="#L404">404</span>
<span id="L405" rel="#L405">405</span>
<span id="L406" rel="#L406">406</span>
<span id="L407" rel="#L407">407</span>
<span id="L408" rel="#L408">408</span>
<span id="L409" rel="#L409">409</span>
<span id="L410" rel="#L410">410</span>
<span id="L411" rel="#L411">411</span>
<span id="L412" rel="#L412">412</span>
<span id="L413" rel="#L413">413</span>
<span id="L414" rel="#L414">414</span>
<span id="L415" rel="#L415">415</span>
<span id="L416" rel="#L416">416</span>
<span id="L417" rel="#L417">417</span>
<span id="L418" rel="#L418">418</span>
<span id="L419" rel="#L419">419</span>
<span id="L420" rel="#L420">420</span>
<span id="L421" rel="#L421">421</span>
<span id="L422" rel="#L422">422</span>
<span id="L423" rel="#L423">423</span>
<span id="L424" rel="#L424">424</span>
<span id="L425" rel="#L425">425</span>
<span id="L426" rel="#L426">426</span>
<span id="L427" rel="#L427">427</span>
<span id="L428" rel="#L428">428</span>
<span id="L429" rel="#L429">429</span>
<span id="L430" rel="#L430">430</span>
<span id="L431" rel="#L431">431</span>
<span id="L432" rel="#L432">432</span>
<span id="L433" rel="#L433">433</span>
<span id="L434" rel="#L434">434</span>
<span id="L435" rel="#L435">435</span>
<span id="L436" rel="#L436">436</span>
<span id="L437" rel="#L437">437</span>
<span id="L438" rel="#L438">438</span>
<span id="L439" rel="#L439">439</span>
<span id="L440" rel="#L440">440</span>
<span id="L441" rel="#L441">441</span>
<span id="L442" rel="#L442">442</span>
<span id="L443" rel="#L443">443</span>
<span id="L444" rel="#L444">444</span>
<span id="L445" rel="#L445">445</span>
<span id="L446" rel="#L446">446</span>
<span id="L447" rel="#L447">447</span>
<span id="L448" rel="#L448">448</span>
<span id="L449" rel="#L449">449</span>
<span id="L450" rel="#L450">450</span>
<span id="L451" rel="#L451">451</span>
<span id="L452" rel="#L452">452</span>
<span id="L453" rel="#L453">453</span>
<span id="L454" rel="#L454">454</span>
<span id="L455" rel="#L455">455</span>
<span id="L456" rel="#L456">456</span>
<span id="L457" rel="#L457">457</span>
<span id="L458" rel="#L458">458</span>
<span id="L459" rel="#L459">459</span>
<span id="L460" rel="#L460">460</span>
<span id="L461" rel="#L461">461</span>
<span id="L462" rel="#L462">462</span>
<span id="L463" rel="#L463">463</span>
</pre>
          </td>
          <td width="100%">
            
              
                <div class="highlight"><pre><div class='line' id='LC1'><span class="c1">// Copyright (c) 2009-2010 Satoshi Nakamoto</span></div><div class='line' id='LC2'><span class="c1">// Distributed under the MIT/X11 software license, see the accompanying</span></div><div class='line' id='LC3'><span class="c1">// file license.txt or http://www.opensource.org/licenses/mit-license.php.</span></div><div class='line' id='LC4'><br/></div><div class='line' id='LC5'><span class="cp">#include &quot;headers.h&quot;</span></div><div class='line' id='LC6'><br/></div><div class='line' id='LC7'><br/></div><div class='line' id='LC8'><br/></div><div class='line' id='LC9'><br/></div><div class='line' id='LC10'><br/></div><div class='line' id='LC11'><br/></div><div class='line' id='LC12'><br/></div><div class='line' id='LC13'><span class="c1">//////////////////////////////////////////////////////////////////////////////</span></div><div class='line' id='LC14'><span class="c1">//</span></div><div class='line' id='LC15'><span class="c1">// Shutdown</span></div><div class='line' id='LC16'><span class="c1">//</span></div><div class='line' id='LC17'><br/></div><div class='line' id='LC18'><span class="kt">void</span> <span class="n">ExitTimeout</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC19'><span class="p">{</span></div><div class='line' id='LC20'><span class="cp">#ifdef __WXMSW__</span></div><div class='line' id='LC21'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">5000</span><span class="p">);</span></div><div class='line' id='LC22'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ExitProcess</span><span class="p">(</span><span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC23'><span class="cp">#endif</span></div><div class='line' id='LC24'><span class="p">}</span></div><div class='line' id='LC25'><br/></div><div class='line' id='LC26'><span class="kt">void</span> <span class="n">Shutdown</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC27'><span class="p">{</span></div><div class='line' id='LC28'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">static</span> <span class="n">CCriticalSection</span> <span class="n">cs_Shutdown</span><span class="p">;</span></div><div class='line' id='LC29'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">static</span> <span class="kt">bool</span> <span class="n">fTaken</span><span class="p">;</span></div><div class='line' id='LC30'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fFirstThread</span><span class="p">;</span></div><div class='line' id='LC31'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_Shutdown</span><span class="p">)</span></div><div class='line' id='LC32'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC33'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fFirstThread</span> <span class="o">=</span> <span class="o">!</span><span class="n">fTaken</span><span class="p">;</span></div><div class='line' id='LC34'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fTaken</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC35'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC36'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">static</span> <span class="kt">bool</span> <span class="n">fExit</span><span class="p">;</span></div><div class='line' id='LC37'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fFirstThread</span><span class="p">)</span></div><div class='line' id='LC38'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC39'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fShutdown</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC40'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nTransactionsUpdated</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC41'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">DBFlush</span><span class="p">(</span><span class="kc">false</span><span class="p">);</span></div><div class='line' id='LC42'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">StopNode</span><span class="p">();</span></div><div class='line' id='LC43'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">DBFlush</span><span class="p">(</span><span class="kc">true</span><span class="p">);</span></div><div class='line' id='LC44'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CreateThread</span><span class="p">(</span><span class="n">ExitTimeout</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">);</span></div><div class='line' id='LC45'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">50</span><span class="p">);</span></div><div class='line' id='LC46'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Bitcoin exiting</span><span class="se">\n\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC47'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fExit</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC48'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">exit</span><span class="p">(</span><span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC49'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC50'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC51'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC52'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">while</span> <span class="p">(</span><span class="o">!</span><span class="n">fExit</span><span class="p">)</span></div><div class='line' id='LC53'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">500</span><span class="p">);</span></div><div class='line' id='LC54'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">100</span><span class="p">);</span></div><div class='line' id='LC55'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ExitThread</span><span class="p">(</span><span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC56'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC57'><span class="p">}</span></div><div class='line' id='LC58'><br/></div><div class='line' id='LC59'><span class="kt">void</span> <span class="n">HandleSIGTERM</span><span class="p">(</span><span class="kt">int</span><span class="p">)</span></div><div class='line' id='LC60'><span class="p">{</span></div><div class='line' id='LC61'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fRequestShutdown</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC62'><span class="p">}</span></div><div class='line' id='LC63'><br/></div><div class='line' id='LC64'><br/></div><div class='line' id='LC65'><br/></div><div class='line' id='LC66'><br/></div><div class='line' id='LC67'><br/></div><div class='line' id='LC68'><br/></div><div class='line' id='LC69'><span class="c1">//////////////////////////////////////////////////////////////////////////////</span></div><div class='line' id='LC70'><span class="c1">//</span></div><div class='line' id='LC71'><span class="c1">// Start</span></div><div class='line' id='LC72'><span class="c1">//</span></div><div class='line' id='LC73'><br/></div><div class='line' id='LC74'><span class="cp">#ifndef GUI</span></div><div class='line' id='LC75'><span class="kt">int</span> <span class="n">main</span><span class="p">(</span><span class="kt">int</span> <span class="n">argc</span><span class="p">,</span> <span class="kt">char</span><span class="o">*</span> <span class="n">argv</span><span class="p">[])</span></div><div class='line' id='LC76'><span class="p">{</span></div><div class='line' id='LC77'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">1</span><span class="p">;</span> <span class="n">i</span> <span class="o">&lt;</span> <span class="n">argc</span><span class="p">;</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC78'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">IsSwitchChar</span><span class="p">(</span><span class="n">argv</span><span class="p">[</span><span class="n">i</span><span class="p">][</span><span class="mi">0</span><span class="p">]))</span></div><div class='line' id='LC79'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fCommandLine</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC80'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fDaemon</span> <span class="o">=</span> <span class="o">!</span><span class="n">fCommandLine</span><span class="p">;</span></div><div class='line' id='LC81'><br/></div><div class='line' id='LC82'><span class="cp">#ifdef __WXGTK__</span></div><div class='line' id='LC83'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">fCommandLine</span><span class="p">)</span></div><div class='line' id='LC84'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC85'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Daemonize</span></div><div class='line' id='LC86'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pid_t</span> <span class="n">pid</span> <span class="o">=</span> <span class="n">fork</span><span class="p">();</span></div><div class='line' id='LC87'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pid</span> <span class="o">&lt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC88'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC89'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fprintf</span><span class="p">(</span><span class="n">stderr</span><span class="p">,</span> <span class="s">&quot;Error: fork() returned %d errno %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">pid</span><span class="p">,</span> <span class="n">errno</span><span class="p">);</span></div><div class='line' id='LC90'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="mi">1</span><span class="p">;</span></div><div class='line' id='LC91'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC92'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pid</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC93'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pthread_exit</span><span class="p">((</span><span class="kt">void</span><span class="o">*</span><span class="p">)</span><span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC94'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC95'><span class="cp">#endif</span></div><div class='line' id='LC96'><br/></div><div class='line' id='LC97'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">AppInit</span><span class="p">(</span><span class="n">argc</span><span class="p">,</span> <span class="n">argv</span><span class="p">))</span></div><div class='line' id='LC98'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="mi">1</span><span class="p">;</span></div><div class='line' id='LC99'><br/></div><div class='line' id='LC100'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">while</span> <span class="p">(</span><span class="o">!</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC101'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">1000000</span><span class="p">);</span></div><div class='line' id='LC102'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC103'><span class="p">}</span></div><div class='line' id='LC104'><span class="cp">#endif</span></div><div class='line' id='LC105'><br/></div><div class='line' id='LC106'><span class="kt">bool</span> <span class="n">AppInit</span><span class="p">(</span><span class="kt">int</span> <span class="n">argc</span><span class="p">,</span> <span class="kt">char</span><span class="o">*</span> <span class="n">argv</span><span class="p">[])</span></div><div class='line' id='LC107'><span class="p">{</span></div><div class='line' id='LC108'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fRet</span> <span class="o">=</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC109'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">try</span></div><div class='line' id='LC110'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC111'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fRet</span> <span class="o">=</span> <span class="n">AppInit2</span><span class="p">(</span><span class="n">argc</span><span class="p">,</span> <span class="n">argv</span><span class="p">);</span></div><div class='line' id='LC112'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC113'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">catch</span> <span class="p">(</span><span class="n">std</span><span class="o">::</span><span class="n">exception</span><span class="o">&amp;</span> <span class="n">e</span><span class="p">)</span> <span class="p">{</span></div><div class='line' id='LC114'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="o">&amp;</span><span class="n">e</span><span class="p">,</span> <span class="s">&quot;AppInit()&quot;</span><span class="p">);</span></div><div class='line' id='LC115'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span> <span class="k">catch</span> <span class="p">(...)</span> <span class="p">{</span></div><div class='line' id='LC116'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="nb">NULL</span><span class="p">,</span> <span class="s">&quot;AppInit()&quot;</span><span class="p">);</span></div><div class='line' id='LC117'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC118'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">fRet</span><span class="p">)</span></div><div class='line' id='LC119'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Shutdown</span><span class="p">(</span><span class="nb">NULL</span><span class="p">);</span></div><div class='line' id='LC120'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">fRet</span><span class="p">;</span></div><div class='line' id='LC121'><span class="p">}</span></div><div class='line' id='LC122'><br/></div><div class='line' id='LC123'><span class="kt">bool</span> <span class="n">AppInit2</span><span class="p">(</span><span class="kt">int</span> <span class="n">argc</span><span class="p">,</span> <span class="kt">char</span><span class="o">*</span> <span class="n">argv</span><span class="p">[])</span></div><div class='line' id='LC124'><span class="p">{</span></div><div class='line' id='LC125'><span class="cp">#ifdef _MSC_VER</span></div><div class='line' id='LC126'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Turn off microsoft heap dump noise</span></div><div class='line' id='LC127'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">_CrtSetReportMode</span><span class="p">(</span><span class="n">_CRT_WARN</span><span class="p">,</span> <span class="n">_CRTDBG_MODE_FILE</span><span class="p">);</span></div><div class='line' id='LC128'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">_CrtSetReportFile</span><span class="p">(</span><span class="n">_CRT_WARN</span><span class="p">,</span> <span class="n">CreateFileA</span><span class="p">(</span><span class="s">&quot;NUL&quot;</span><span class="p">,</span> <span class="n">GENERIC_WRITE</span><span class="p">,</span> <span class="mi">0</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">,</span> <span class="n">OPEN_EXISTING</span><span class="p">,</span> <span class="mi">0</span><span class="p">,</span> <span class="mi">0</span><span class="p">));</span></div><div class='line' id='LC129'><span class="cp">#endif</span></div><div class='line' id='LC130'><span class="cp">#if _MSC_VER &gt;= 1400</span></div><div class='line' id='LC131'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Disable confusing &quot;helpful&quot; text message on abort, ctrl-c</span></div><div class='line' id='LC132'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">_set_abort_behavior</span><span class="p">(</span><span class="mi">0</span><span class="p">,</span> <span class="n">_WRITE_ABORT_MSG</span> <span class="o">|</span> <span class="n">_CALL_REPORTFAULT</span><span class="p">);</span></div><div class='line' id='LC133'><span class="cp">#endif</span></div><div class='line' id='LC134'><span class="cp">#ifndef __WXMSW__</span></div><div class='line' id='LC135'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">umask</span><span class="p">(</span><span class="mo">077</span><span class="p">);</span></div><div class='line' id='LC136'><span class="cp">#endif</span></div><div class='line' id='LC137'><span class="cp">#ifndef __WXMSW__</span></div><div class='line' id='LC138'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Clean shutdown on SIGTERM</span></div><div class='line' id='LC139'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">sigaction</span> <span class="n">sa</span><span class="p">;</span></div><div class='line' id='LC140'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sa</span><span class="p">.</span><span class="n">sa_handler</span> <span class="o">=</span> <span class="n">HandleSIGTERM</span><span class="p">;</span></div><div class='line' id='LC141'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sigemptyset</span><span class="p">(</span><span class="o">&amp;</span><span class="n">sa</span><span class="p">.</span><span class="n">sa_mask</span><span class="p">);</span></div><div class='line' id='LC142'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sa</span><span class="p">.</span><span class="n">sa_flags</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC143'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sigaction</span><span class="p">(</span><span class="n">SIGTERM</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">sa</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">);</span></div><div class='line' id='LC144'><span class="cp">#endif</span></div><div class='line' id='LC145'><br/></div><div class='line' id='LC146'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC147'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Parameters</span></div><div class='line' id='LC148'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC149'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ParseParameters</span><span class="p">(</span><span class="n">argc</span><span class="p">,</span> <span class="n">argv</span><span class="p">);</span></div><div class='line' id='LC150'><br/></div><div class='line' id='LC151'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-datadir&quot;</span><span class="p">))</span></div><div class='line' id='LC152'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC153'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">filesystem</span><span class="o">::</span><span class="n">path</span> <span class="n">pathDataDir</span> <span class="o">=</span> <span class="n">filesystem</span><span class="o">::</span><span class="n">system_complete</span><span class="p">(</span><span class="n">mapArgs</span><span class="p">[</span><span class="s">&quot;-datadir&quot;</span><span class="p">]);</span></div><div class='line' id='LC154'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strlcpy</span><span class="p">(</span><span class="n">pszSetDataDir</span><span class="p">,</span> <span class="n">pathDataDir</span><span class="p">.</span><span class="n">string</span><span class="p">().</span><span class="n">c_str</span><span class="p">(),</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">pszSetDataDir</span><span class="p">));</span></div><div class='line' id='LC155'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC156'><br/></div><div class='line' id='LC157'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ReadConfigFile</span><span class="p">(</span><span class="n">mapArgs</span><span class="p">,</span> <span class="n">mapMultiArgs</span><span class="p">);</span> <span class="c1">// Must be done after processing datadir</span></div><div class='line' id='LC158'><br/></div><div class='line' id='LC159'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-?&quot;</span><span class="p">)</span> <span class="o">||</span> <span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;--help&quot;</span><span class="p">))</span></div><div class='line' id='LC160'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC161'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">string</span> <span class="n">beta</span> <span class="o">=</span> <span class="n">VERSION_IS_BETA</span> <span class="o">?</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot; beta&quot;</span><span class="p">)</span> <span class="o">:</span> <span class="s">&quot;&quot;</span><span class="p">;</span></div><div class='line' id='LC162'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">string</span> <span class="n">strUsage</span> <span class="o">=</span> <span class="n">string</span><span class="p">()</span> <span class="o">+</span></div><div class='line' id='LC163'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">_</span><span class="p">(</span><span class="s">&quot;Bitcoin version&quot;</span><span class="p">)</span> <span class="o">+</span> <span class="s">&quot; &quot;</span> <span class="o">+</span> <span class="n">FormatVersion</span><span class="p">(</span><span class="n">VERSION</span><span class="p">)</span> <span class="o">+</span> <span class="n">pszSubVer</span> <span class="o">+</span> <span class="n">beta</span> <span class="o">+</span> <span class="s">&quot;</span><span class="se">\n\n</span><span class="s">&quot;</span> <span class="o">+</span></div><div class='line' id='LC164'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">_</span><span class="p">(</span><span class="s">&quot;Usage:&quot;</span><span class="p">)</span> <span class="o">+</span> <span class="s">&quot;</span><span class="se">\t\t\t\t\t\t\t\t\t\t\n</span><span class="s">&quot;</span> <span class="o">+</span></div><div class='line' id='LC165'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  bitcoin [options]                   </span><span class="se">\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="s">&quot;</span><span class="se">\n</span><span class="s">&quot;</span> <span class="o">+</span></div><div class='line' id='LC166'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  bitcoin [options] &lt;command&gt; [params]</span><span class="se">\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Send command to -server or bitcoind</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC167'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  bitcoin [options] help              </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;List commands</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC168'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  bitcoin [options] help &lt;command&gt;    </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Get help for a command</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC169'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">_</span><span class="p">(</span><span class="s">&quot;Options:</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC170'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -conf=&lt;file&gt;     </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Specify configuration file (default: bitcoin.conf)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC171'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -gen             </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Generate coins</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC172'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -gen=0           </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Don&#39;t generate coins</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC173'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -min             </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Start minimized</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC174'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -datadir=&lt;dir&gt;   </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Specify data directory</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC175'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -proxy=&lt;ip:port&gt; </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Connect through socks4 proxy</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC176'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -addnode=&lt;ip&gt;    </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Add a node to connect to</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC177'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -connect=&lt;ip&gt;    </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Connect only to the specified node</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC178'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -nolisten        </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Don&#39;t accept connections from outside</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC179'><span class="cp">#ifdef USE_UPNP</span></div><div class='line' id='LC180'><span class="cp">#if USE_UPNP</span></div><div class='line' id='LC181'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -noupnp          </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Don&#39;t attempt to use UPnP to map the listening port</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC182'><span class="cp">#else</span></div><div class='line' id='LC183'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -upnp            </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Attempt to use UPnP to map the listening port</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC184'><span class="cp">#endif</span></div><div class='line' id='LC185'><span class="cp">#endif</span></div><div class='line' id='LC186'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -paytxfee=&lt;amt&gt;  </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Fee per KB to add to transactions you send</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC187'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -server          </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Accept command line and JSON-RPC commands</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC188'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -daemon          </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Run in the background as a daemon and accept commands</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC189'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -testnet         </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Use the test network</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC190'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcuser=&lt;user&gt;  </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Username for JSON-RPC connections</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC191'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcpassword=&lt;pw&gt;</span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Password for JSON-RPC connections</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC192'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcport=&lt;port&gt;  </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Listen for JSON-RPC connections on &lt;port&gt; (default: 8332)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC193'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcallowip=&lt;ip&gt; </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Allow JSON-RPC connections from specified IP address</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC194'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcconnect=&lt;ip&gt; </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Send commands to node running on &lt;ip&gt; (default: 127.0.0.1)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC195'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -keypool=&lt;n&gt;     </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Set key pool size to &lt;n&gt; (default: 100)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC196'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rescan          </span><span class="se">\t</span><span class="s">  &quot;</span>   <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Rescan the block chain for missing wallet transactions</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC197'><br/></div><div class='line' id='LC198'><span class="cp">#ifdef USE_SSL</span></div><div class='line' id='LC199'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strUsage</span> <span class="o">+=</span> <span class="n">string</span><span class="p">()</span> <span class="o">+</span></div><div class='line' id='LC200'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">_</span><span class="p">(</span><span class="s">&quot;</span><span class="se">\n</span><span class="s">SSL options: (see the Bitcoin Wiki for SSL setup instructions)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC201'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcssl                                </span><span class="se">\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Use OpenSSL (https) for JSON-RPC connections</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC202'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcsslcertificatechainfile=&lt;file.cert&gt;</span><span class="se">\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Server certificate file (default: server.cert)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC203'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcsslprivatekeyfile=&lt;file.pem&gt;       </span><span class="se">\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Server private key (default: server.pem)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">)</span> <span class="o">+</span></div><div class='line' id='LC204'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -rpcsslciphers=&lt;ciphers&gt;               </span><span class="se">\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC205'><span class="cp">#endif</span></div><div class='line' id='LC206'><br/></div><div class='line' id='LC207'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strUsage</span> <span class="o">+=</span> <span class="n">string</span><span class="p">()</span> <span class="o">+</span></div><div class='line' id='LC208'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;  -?               </span><span class="se">\t\t</span><span class="s">  &quot;</span> <span class="o">+</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;This help message</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC209'><br/></div><div class='line' id='LC210'><span class="cp">#if defined(__WXMSW__) &amp;&amp; defined(GUI)</span></div><div class='line' id='LC211'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Tabs make the columns line up in the message box</span></div><div class='line' id='LC212'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="n">strUsage</span><span class="p">,</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">,</span> <span class="n">wxOK</span><span class="p">);</span></div><div class='line' id='LC213'><span class="cp">#else</span></div><div class='line' id='LC214'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Remove tabs</span></div><div class='line' id='LC215'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strUsage</span><span class="p">.</span><span class="n">erase</span><span class="p">(</span><span class="n">std</span><span class="o">::</span><span class="n">remove</span><span class="p">(</span><span class="n">strUsage</span><span class="p">.</span><span class="n">begin</span><span class="p">(),</span> <span class="n">strUsage</span><span class="p">.</span><span class="n">end</span><span class="p">(),</span> <span class="sc">&#39;\t&#39;</span><span class="p">),</span> <span class="n">strUsage</span><span class="p">.</span><span class="n">end</span><span class="p">());</span></div><div class='line' id='LC216'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fprintf</span><span class="p">(</span><span class="n">stderr</span><span class="p">,</span> <span class="s">&quot;%s&quot;</span><span class="p">,</span> <span class="n">strUsage</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC217'><span class="cp">#endif</span></div><div class='line' id='LC218'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC219'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC220'><br/></div><div class='line' id='LC221'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fDebug</span> <span class="o">=</span> <span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-debug&quot;</span><span class="p">);</span></div><div class='line' id='LC222'><br/></div><div class='line' id='LC223'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fPrintToConsole</span> <span class="o">=</span> <span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-printtoconsole&quot;</span><span class="p">);</span></div><div class='line' id='LC224'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fPrintToDebugger</span> <span class="o">=</span> <span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-printtodebugger&quot;</span><span class="p">);</span></div><div class='line' id='LC225'><br/></div><div class='line' id='LC226'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fTestNet</span> <span class="o">=</span> <span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-testnet&quot;</span><span class="p">);</span></div><div class='line' id='LC227'>&nbsp;&nbsp;&nbsp;&nbsp;</div><div class='line' id='LC228'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fNoListen</span> <span class="o">=</span> <span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-nolisten&quot;</span><span class="p">);</span></div><div class='line' id='LC229'><br/></div><div class='line' id='LC230'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fCommandLine</span><span class="p">)</span></div><div class='line' id='LC231'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC232'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">ret</span> <span class="o">=</span> <span class="n">CommandLineRPC</span><span class="p">(</span><span class="n">argc</span><span class="p">,</span> <span class="n">argv</span><span class="p">);</span></div><div class='line' id='LC233'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">exit</span><span class="p">(</span><span class="n">ret</span><span class="p">);</span></div><div class='line' id='LC234'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC235'><br/></div><div class='line' id='LC236'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">fDebug</span> <span class="o">&amp;&amp;</span> <span class="o">!</span><span class="n">pszSetDataDir</span><span class="p">[</span><span class="mi">0</span><span class="p">])</span></div><div class='line' id='LC237'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ShrinkDebugFile</span><span class="p">();</span></div><div class='line' id='LC238'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;</span><span class="se">\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC239'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Bitcoin version %s%s%s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">FormatVersion</span><span class="p">(</span><span class="n">VERSION</span><span class="p">).</span><span class="n">c_str</span><span class="p">(),</span> <span class="n">pszSubVer</span><span class="p">,</span> <span class="n">VERSION_IS_BETA</span> <span class="o">?</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot; beta&quot;</span><span class="p">)</span> <span class="o">:</span> <span class="s">&quot;&quot;</span><span class="p">);</span></div><div class='line' id='LC240'><span class="cp">#ifdef GUI</span></div><div class='line' id='LC241'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;OS version %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="p">((</span><span class="n">string</span><span class="p">)</span><span class="n">wxGetOsDescription</span><span class="p">()).</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC242'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;System default language is %d %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">g_locale</span><span class="p">.</span><span class="n">GetSystemLanguage</span><span class="p">(),</span> <span class="p">((</span><span class="n">string</span><span class="p">)</span><span class="n">g_locale</span><span class="p">.</span><span class="n">GetSysName</span><span class="p">()).</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC243'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Language file %s (%s)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="p">(</span><span class="n">string</span><span class="p">(</span><span class="s">&quot;locale/&quot;</span><span class="p">)</span> <span class="o">+</span> <span class="p">(</span><span class="n">string</span><span class="p">)</span><span class="n">g_locale</span><span class="p">.</span><span class="n">GetCanonicalName</span><span class="p">()</span> <span class="o">+</span> <span class="s">&quot;/LC_MESSAGES/bitcoin.mo&quot;</span><span class="p">).</span><span class="n">c_str</span><span class="p">(),</span> <span class="p">((</span><span class="n">string</span><span class="p">)</span><span class="n">g_locale</span><span class="p">.</span><span class="n">GetLocale</span><span class="p">()).</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC244'><span class="cp">#endif</span></div><div class='line' id='LC245'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Default data directory %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">GetDefaultDataDir</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC246'><br/></div><div class='line' id='LC247'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-loadblockindextest&quot;</span><span class="p">))</span></div><div class='line' id='LC248'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC249'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CTxDB</span> <span class="n">txdb</span><span class="p">(</span><span class="s">&quot;r&quot;</span><span class="p">);</span></div><div class='line' id='LC250'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">txdb</span><span class="p">.</span><span class="n">LoadBlockIndex</span><span class="p">();</span></div><div class='line' id='LC251'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintBlockTree</span><span class="p">();</span></div><div class='line' id='LC252'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC253'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC254'><br/></div><div class='line' id='LC255'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC256'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Limit to single instance per user</span></div><div class='line' id='LC257'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Required to protect the database files if we&#39;re going to keep deleting log.*</span></div><div class='line' id='LC258'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC259'><span class="cp">#if defined(__WXMSW__) &amp;&amp; defined(GUI)</span></div><div class='line' id='LC260'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// wxSingleInstanceChecker doesn&#39;t work on Linux</span></div><div class='line' id='LC261'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxString</span> <span class="n">strMutexName</span> <span class="o">=</span> <span class="n">wxString</span><span class="p">(</span><span class="s">&quot;bitcoin_running.&quot;</span><span class="p">)</span> <span class="o">+</span> <span class="n">getenv</span><span class="p">(</span><span class="s">&quot;HOMEPATH&quot;</span><span class="p">);</span></div><div class='line' id='LC262'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">i</span> <span class="o">&lt;</span> <span class="n">strMutexName</span><span class="p">.</span><span class="n">size</span><span class="p">();</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC263'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">isalnum</span><span class="p">(</span><span class="n">strMutexName</span><span class="p">[</span><span class="n">i</span><span class="p">]))</span></div><div class='line' id='LC264'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strMutexName</span><span class="p">[</span><span class="n">i</span><span class="p">]</span> <span class="o">=</span> <span class="sc">&#39;.&#39;</span><span class="p">;</span></div><div class='line' id='LC265'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxSingleInstanceChecker</span><span class="o">*</span> <span class="n">psingleinstancechecker</span> <span class="o">=</span> <span class="k">new</span> <span class="n">wxSingleInstanceChecker</span><span class="p">(</span><span class="n">strMutexName</span><span class="p">);</span></div><div class='line' id='LC266'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">psingleinstancechecker</span><span class="o">-&gt;</span><span class="n">IsAnotherRunning</span><span class="p">())</span></div><div class='line' id='LC267'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC268'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Existing instance found</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC269'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nStart</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC270'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">loop</span></div><div class='line' id='LC271'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC272'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Show the previous instance and exit</span></div><div class='line' id='LC273'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">HWND</span> <span class="n">hwndPrev</span> <span class="o">=</span> <span class="n">FindWindowA</span><span class="p">(</span><span class="s">&quot;wxWindowClassNR&quot;</span><span class="p">,</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">);</span></div><div class='line' id='LC274'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">hwndPrev</span><span class="p">)</span></div><div class='line' id='LC275'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC276'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">IsIconic</span><span class="p">(</span><span class="n">hwndPrev</span><span class="p">))</span></div><div class='line' id='LC277'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ShowWindow</span><span class="p">(</span><span class="n">hwndPrev</span><span class="p">,</span> <span class="n">SW_RESTORE</span><span class="p">);</span></div><div class='line' id='LC278'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SetForegroundWindow</span><span class="p">(</span><span class="n">hwndPrev</span><span class="p">);</span></div><div class='line' id='LC279'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC280'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC281'><br/></div><div class='line' id='LC282'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetTime</span><span class="p">()</span> <span class="o">&gt;</span> <span class="n">nStart</span> <span class="o">+</span> <span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC283'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC284'><br/></div><div class='line' id='LC285'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Resume this instance if the other exits</span></div><div class='line' id='LC286'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">delete</span> <span class="n">psingleinstancechecker</span><span class="p">;</span></div><div class='line' id='LC287'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">1000</span><span class="p">);</span></div><div class='line' id='LC288'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">psingleinstancechecker</span> <span class="o">=</span> <span class="k">new</span> <span class="n">wxSingleInstanceChecker</span><span class="p">(</span><span class="n">strMutexName</span><span class="p">);</span></div><div class='line' id='LC289'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">psingleinstancechecker</span><span class="o">-&gt;</span><span class="n">IsAnotherRunning</span><span class="p">())</span></div><div class='line' id='LC290'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">break</span><span class="p">;</span></div><div class='line' id='LC291'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC292'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC293'><span class="cp">#endif</span></div><div class='line' id='LC294'><br/></div><div class='line' id='LC295'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Make sure only a single bitcoin process is using the data directory.</span></div><div class='line' id='LC296'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">string</span> <span class="n">strLockFile</span> <span class="o">=</span> <span class="n">GetDataDir</span><span class="p">()</span> <span class="o">+</span> <span class="s">&quot;/.lock&quot;</span><span class="p">;</span></div><div class='line' id='LC297'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FILE</span><span class="o">*</span> <span class="n">file</span> <span class="o">=</span> <span class="n">fopen</span><span class="p">(</span><span class="n">strLockFile</span><span class="p">.</span><span class="n">c_str</span><span class="p">(),</span> <span class="s">&quot;a&quot;</span><span class="p">);</span> <span class="c1">// empty lock file; created if it doesn&#39;t exist.</span></div><div class='line' id='LC298'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fclose</span><span class="p">(</span><span class="n">file</span><span class="p">);</span></div><div class='line' id='LC299'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">static</span> <span class="n">boost</span><span class="o">::</span><span class="n">interprocess</span><span class="o">::</span><span class="n">file_lock</span> <span class="n">lock</span><span class="p">(</span><span class="n">strLockFile</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC300'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">lock</span><span class="p">.</span><span class="n">try_lock</span><span class="p">())</span></div><div class='line' id='LC301'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC302'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="n">strprintf</span><span class="p">(</span><span class="n">_</span><span class="p">(</span><span class="s">&quot;Cannot obtain a lock on data directory %s.  Bitcoin is probably already running.&quot;</span><span class="p">),</span> <span class="n">GetDataDir</span><span class="p">().</span><span class="n">c_str</span><span class="p">()),</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">);</span></div><div class='line' id='LC303'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC304'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC305'><br/></div><div class='line' id='LC306'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Bind to the port early so we can tell if another instance is already running.</span></div><div class='line' id='LC307'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">string</span> <span class="n">strErrors</span><span class="p">;</span></div><div class='line' id='LC308'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">fNoListen</span><span class="p">)</span></div><div class='line' id='LC309'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC310'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">BindListenPort</span><span class="p">(</span><span class="n">strErrors</span><span class="p">))</span></div><div class='line' id='LC311'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC312'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="n">strErrors</span><span class="p">,</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">);</span></div><div class='line' id='LC313'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC314'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC315'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC316'><br/></div><div class='line' id='LC317'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC318'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Load data files</span></div><div class='line' id='LC319'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC320'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fDaemon</span><span class="p">)</span></div><div class='line' id='LC321'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fprintf</span><span class="p">(</span><span class="n">stdout</span><span class="p">,</span> <span class="s">&quot;bitcoin server starting</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC322'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strErrors</span> <span class="o">=</span> <span class="s">&quot;&quot;</span><span class="p">;</span></div><div class='line' id='LC323'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nStart</span><span class="p">;</span></div><div class='line' id='LC324'><br/></div><div class='line' id='LC325'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Loading addresses...</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC326'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nStart</span> <span class="o">=</span> <span class="n">GetTimeMillis</span><span class="p">();</span></div><div class='line' id='LC327'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">LoadAddresses</span><span class="p">())</span></div><div class='line' id='LC328'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strErrors</span> <span class="o">+=</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Error loading addr.dat      </span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC329'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot; addresses   %15&quot;</span><span class="n">PRI64d</span><span class="s">&quot;ms</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">GetTimeMillis</span><span class="p">()</span> <span class="o">-</span> <span class="n">nStart</span><span class="p">);</span></div><div class='line' id='LC330'><br/></div><div class='line' id='LC331'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Loading block index...</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC332'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nStart</span> <span class="o">=</span> <span class="n">GetTimeMillis</span><span class="p">();</span></div><div class='line' id='LC333'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">LoadBlockIndex</span><span class="p">())</span></div><div class='line' id='LC334'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strErrors</span> <span class="o">+=</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Error loading blkindex.dat      </span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC335'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot; block index %15&quot;</span><span class="n">PRI64d</span><span class="s">&quot;ms</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">GetTimeMillis</span><span class="p">()</span> <span class="o">-</span> <span class="n">nStart</span><span class="p">);</span></div><div class='line' id='LC336'><br/></div><div class='line' id='LC337'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Loading wallet...</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC338'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nStart</span> <span class="o">=</span> <span class="n">GetTimeMillis</span><span class="p">();</span></div><div class='line' id='LC339'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fFirstRun</span><span class="p">;</span></div><div class='line' id='LC340'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">LoadWallet</span><span class="p">(</span><span class="n">fFirstRun</span><span class="p">))</span></div><div class='line' id='LC341'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strErrors</span> <span class="o">+=</span> <span class="n">_</span><span class="p">(</span><span class="s">&quot;Error loading wallet.dat      </span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC342'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot; wallet      %15&quot;</span><span class="n">PRI64d</span><span class="s">&quot;ms</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">GetTimeMillis</span><span class="p">()</span> <span class="o">-</span> <span class="n">nStart</span><span class="p">);</span></div><div class='line' id='LC343'><br/></div><div class='line' id='LC344'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-rescan&quot;</span><span class="p">))</span></div><div class='line' id='LC345'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC346'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nStart</span> <span class="o">=</span> <span class="n">GetTimeMillis</span><span class="p">();</span></div><div class='line' id='LC347'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ScanForWalletTransactions</span><span class="p">(</span><span class="n">pindexGenesisBlock</span><span class="p">);</span></div><div class='line' id='LC348'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot; rescan      %15&quot;</span><span class="n">PRI64d</span><span class="s">&quot;ms</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">GetTimeMillis</span><span class="p">()</span> <span class="o">-</span> <span class="n">nStart</span><span class="p">);</span></div><div class='line' id='LC349'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC350'><br/></div><div class='line' id='LC351'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Done loading</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC352'><br/></div><div class='line' id='LC353'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//// debug print</span></div><div class='line' id='LC354'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;mapBlockIndex.size() = %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span>   <span class="n">mapBlockIndex</span><span class="p">.</span><span class="n">size</span><span class="p">());</span></div><div class='line' id='LC355'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;nBestHeight = %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span>            <span class="n">nBestHeight</span><span class="p">);</span></div><div class='line' id='LC356'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;mapKeys.size() = %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span>         <span class="n">mapKeys</span><span class="p">.</span><span class="n">size</span><span class="p">());</span></div><div class='line' id='LC357'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;mapPubKeys.size() = %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span>      <span class="n">mapPubKeys</span><span class="p">.</span><span class="n">size</span><span class="p">());</span></div><div class='line' id='LC358'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;mapWallet.size() = %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span>       <span class="n">mapWallet</span><span class="p">.</span><span class="n">size</span><span class="p">());</span></div><div class='line' id='LC359'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;mapAddressBook.size() = %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span>  <span class="n">mapAddressBook</span><span class="p">.</span><span class="n">size</span><span class="p">());</span></div><div class='line' id='LC360'><br/></div><div class='line' id='LC361'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">strErrors</span><span class="p">.</span><span class="n">empty</span><span class="p">())</span></div><div class='line' id='LC362'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC363'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="n">strErrors</span><span class="p">,</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">,</span> <span class="n">wxOK</span> <span class="o">|</span> <span class="n">wxICON_ERROR</span><span class="p">);</span></div><div class='line' id='LC364'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC365'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC366'><br/></div><div class='line' id='LC367'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Add wallet transactions that aren&#39;t already in a block to mapTransactions</span></div><div class='line' id='LC368'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ReacceptWalletTransactions</span><span class="p">();</span></div><div class='line' id='LC369'><br/></div><div class='line' id='LC370'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC371'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Parameters</span></div><div class='line' id='LC372'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC373'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-printblockindex&quot;</span><span class="p">)</span> <span class="o">||</span> <span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-printblocktree&quot;</span><span class="p">))</span></div><div class='line' id='LC374'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC375'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintBlockTree</span><span class="p">();</span></div><div class='line' id='LC376'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC377'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC378'><br/></div><div class='line' id='LC379'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-printblock&quot;</span><span class="p">))</span></div><div class='line' id='LC380'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC381'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">string</span> <span class="n">strMatch</span> <span class="o">=</span> <span class="n">mapArgs</span><span class="p">[</span><span class="s">&quot;-printblock&quot;</span><span class="p">];</span></div><div class='line' id='LC382'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nFound</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC383'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="n">map</span><span class="o">&lt;</span><span class="n">uint256</span><span class="p">,</span> <span class="n">CBlockIndex</span><span class="o">*&gt;::</span><span class="n">iterator</span> <span class="n">mi</span> <span class="o">=</span> <span class="n">mapBlockIndex</span><span class="p">.</span><span class="n">begin</span><span class="p">();</span> <span class="n">mi</span> <span class="o">!=</span> <span class="n">mapBlockIndex</span><span class="p">.</span><span class="n">end</span><span class="p">();</span> <span class="o">++</span><span class="n">mi</span><span class="p">)</span></div><div class='line' id='LC384'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC385'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">uint256</span> <span class="n">hash</span> <span class="o">=</span> <span class="p">(</span><span class="o">*</span><span class="n">mi</span><span class="p">).</span><span class="n">first</span><span class="p">;</span></div><div class='line' id='LC386'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">strncmp</span><span class="p">(</span><span class="n">hash</span><span class="p">.</span><span class="n">ToString</span><span class="p">().</span><span class="n">c_str</span><span class="p">(),</span> <span class="n">strMatch</span><span class="p">.</span><span class="n">c_str</span><span class="p">(),</span> <span class="n">strMatch</span><span class="p">.</span><span class="n">size</span><span class="p">())</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC387'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC388'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CBlockIndex</span><span class="o">*</span> <span class="n">pindex</span> <span class="o">=</span> <span class="p">(</span><span class="o">*</span><span class="n">mi</span><span class="p">).</span><span class="n">second</span><span class="p">;</span></div><div class='line' id='LC389'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CBlock</span> <span class="n">block</span><span class="p">;</span></div><div class='line' id='LC390'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">block</span><span class="p">.</span><span class="n">ReadFromDisk</span><span class="p">(</span><span class="n">pindex</span><span class="p">);</span></div><div class='line' id='LC391'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">block</span><span class="p">.</span><span class="n">BuildMerkleTree</span><span class="p">();</span></div><div class='line' id='LC392'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">block</span><span class="p">.</span><span class="n">print</span><span class="p">();</span></div><div class='line' id='LC393'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC394'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nFound</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC395'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC396'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC397'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nFound</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC398'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;No blocks matching %s were found</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">strMatch</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC399'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC400'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC401'><br/></div><div class='line' id='LC402'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fGenerateBitcoins</span> <span class="o">=</span> <span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-gen&quot;</span><span class="p">);</span></div><div class='line' id='LC403'><br/></div><div class='line' id='LC404'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-proxy&quot;</span><span class="p">))</span></div><div class='line' id='LC405'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC406'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fUseProxy</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC407'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrProxy</span> <span class="o">=</span> <span class="n">CAddress</span><span class="p">(</span><span class="n">mapArgs</span><span class="p">[</span><span class="s">&quot;-proxy&quot;</span><span class="p">]);</span></div><div class='line' id='LC408'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">addrProxy</span><span class="p">.</span><span class="n">IsValid</span><span class="p">())</span></div><div class='line' id='LC409'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC410'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="n">_</span><span class="p">(</span><span class="s">&quot;Invalid -proxy address&quot;</span><span class="p">),</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">);</span></div><div class='line' id='LC411'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC412'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC413'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC414'><br/></div><div class='line' id='LC415'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-addnode&quot;</span><span class="p">))</span></div><div class='line' id='LC416'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC417'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">string</span> <span class="n">strAddr</span><span class="p">,</span> <span class="n">mapMultiArgs</span><span class="p">[</span><span class="s">&quot;-addnode&quot;</span><span class="p">])</span></div><div class='line' id='LC418'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC419'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="n">strAddr</span><span class="p">,</span> <span class="n">NODE_NETWORK</span><span class="p">);</span></div><div class='line' id='LC420'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addr</span><span class="p">.</span><span class="n">nTime</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="c1">// so it won&#39;t relay unless successfully connected</span></div><div class='line' id='LC421'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">IsValid</span><span class="p">())</span></div><div class='line' id='LC422'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">AddAddress</span><span class="p">(</span><span class="n">addr</span><span class="p">);</span></div><div class='line' id='LC423'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC424'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC425'><br/></div><div class='line' id='LC426'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-paytxfee&quot;</span><span class="p">))</span></div><div class='line' id='LC427'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC428'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">ParseMoney</span><span class="p">(</span><span class="n">mapArgs</span><span class="p">[</span><span class="s">&quot;-paytxfee&quot;</span><span class="p">],</span> <span class="n">nTransactionFee</span><span class="p">))</span></div><div class='line' id='LC429'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC430'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="n">_</span><span class="p">(</span><span class="s">&quot;Invalid amount for -paytxfee=&lt;amount&gt;&quot;</span><span class="p">),</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">);</span></div><div class='line' id='LC431'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC432'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC433'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nTransactionFee</span> <span class="o">&gt;</span> <span class="mf">0.25</span> <span class="o">*</span> <span class="n">COIN</span><span class="p">)</span></div><div class='line' id='LC434'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="n">_</span><span class="p">(</span><span class="s">&quot;Warning: -paytxfee is set very high.  This is the transaction fee you will pay if you send a transaction.&quot;</span><span class="p">),</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">,</span> <span class="n">wxOK</span> <span class="o">|</span> <span class="n">wxICON_EXCLAMATION</span><span class="p">);</span></div><div class='line' id='LC435'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC436'><br/></div><div class='line' id='LC437'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC438'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Create the main window and start the node</span></div><div class='line' id='LC439'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC440'><span class="cp">#ifdef GUI</span></div><div class='line' id='LC441'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">fDaemon</span><span class="p">)</span></div><div class='line' id='LC442'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CreateMainWindow</span><span class="p">();</span></div><div class='line' id='LC443'><span class="cp">#endif</span></div><div class='line' id='LC444'><br/></div><div class='line' id='LC445'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">CheckDiskSpace</span><span class="p">())</span></div><div class='line' id='LC446'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC447'><br/></div><div class='line' id='LC448'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">RandAddSeedPerfmon</span><span class="p">();</span></div><div class='line' id='LC449'><br/></div><div class='line' id='LC450'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">CreateThread</span><span class="p">(</span><span class="n">StartNode</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">))</span></div><div class='line' id='LC451'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">wxMessageBox</span><span class="p">(</span><span class="s">&quot;Error: CreateThread(StartNode) failed&quot;</span><span class="p">,</span> <span class="s">&quot;Bitcoin&quot;</span><span class="p">);</span></div><div class='line' id='LC452'><br/></div><div class='line' id='LC453'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-server&quot;</span><span class="p">)</span> <span class="o">||</span> <span class="n">fDaemon</span><span class="p">)</span></div><div class='line' id='LC454'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CreateThread</span><span class="p">(</span><span class="n">ThreadRPCServer</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">);</span></div><div class='line' id='LC455'><br/></div><div class='line' id='LC456'><span class="cp">#if defined(__WXMSW__) &amp;&amp; defined(GUI)</span></div><div class='line' id='LC457'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fFirstRun</span><span class="p">)</span></div><div class='line' id='LC458'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SetStartOnSystemStartup</span><span class="p">(</span><span class="kc">true</span><span class="p">);</span></div><div class='line' id='LC459'><span class="cp">#endif</span></div><div class='line' id='LC460'><br/></div><div class='line' id='LC461'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC462'><span class="p">}</span></div><div class='line' id='LC463'><br/></div></pre></div>
              
            
          </td>
        </tr>
      </table>
    
  </div>


          </div>
        </div>
      </div>
    </div>
  

  </div>


<div class="frame frame-loading" style="display:none;">
  <img src="/images/modules/ajax/big_spinner_336699.gif">
</div>

    </div>
  
      
    </div>

    <div id="footer" class="clearfix">
      <div class="site">
        <div class="sponsor">
          <a href="http://www.rackspace.com" class="logo">
            <img alt="Dedicated Server" src="https://assets0.github.com/images/modules/footer/rackspace_logo.png?v2?2b893794092cd44a789db64b3da95d1091187f55" />
          </a>
          Powered by the <a href="http://www.rackspace.com ">Dedicated
          Servers</a> and<br/> <a href="http://www.rackspacecloud.com">Cloud
          Computing</a> of Rackspace Hosting<span>&reg;</span>
        </div>

        <ul class="links">
          <li class="blog"><a href="https://github.com/blog">Blog</a></li>
          <li><a href="/login/multipass?to=http%3A%2F%2Fsupport.github.com">Support</a></li>
          <li><a href="https://github.com/training">Training</a></li>
          <li><a href="http://jobs.github.com">Job Board</a></li>
          <li><a href="http://shop.github.com">Shop</a></li>
          <li><a href="https://github.com/contact">Contact</a></li>
          <li><a href="http://develop.github.com">API</a></li>
          <li><a href="http://status.github.com">Status</a></li>
        </ul>
        <ul class="sosueme">
          <li class="main">&copy; 2011 <span id="_rrt" title="0.06204s from fe1.rs.github.com">GitHub</span> Inc. All rights reserved.</li>
          <li><a href="/site/terms">Terms of Service</a></li>
          <li><a href="/site/privacy">Privacy</a></li>
          <li><a href="https://github.com/security">Security</a></li>
        </ul>
      </div>
    </div><!-- /#footer -->

    
      
      
        <!-- current locale:  -->
        <div class="locales">
          <div class="site">

            <ul class="choices clearfix limited-locales">
              <li><span class="current">English</span></li>
              
                  <li><a rel="nofollow" href="?locale=de">Deutsch</a></li>
              
                  <li><a rel="nofollow" href="?locale=fr">Français</a></li>
              
                  <li><a rel="nofollow" href="?locale=ja">日本語</a></li>
              
                  <li><a rel="nofollow" href="?locale=pt-BR">Português (BR)</a></li>
              
                  <li><a rel="nofollow" href="?locale=ru">Русский</a></li>
              
                  <li><a rel="nofollow" href="?locale=zh">中文</a></li>
              
              <li class="all"><a href="#" class="minibutton btn-forward js-all-locales"><span><span class="icon"></span>See all available languages</span></a></li>
            </ul>

            <div class="all-locales clearfix">
              <h3>Your current locale selection: <strong>English</strong>. Choose another?</h3>
              
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=en">English</a></li>
                  
                      <li><a rel="nofollow" href="?locale=af">Afrikaans</a></li>
                  
                      <li><a rel="nofollow" href="?locale=ca">Català</a></li>
                  
                      <li><a rel="nofollow" href="?locale=cs">Čeština</a></li>
                  
                      <li><a rel="nofollow" href="?locale=de">Deutsch</a></li>
                  
                </ul>
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=es">Español</a></li>
                  
                      <li><a rel="nofollow" href="?locale=fr">Français</a></li>
                  
                      <li><a rel="nofollow" href="?locale=hr">Hrvatski</a></li>
                  
                      <li><a rel="nofollow" href="?locale=hu">Magyar</a></li>
                  
                      <li><a rel="nofollow" href="?locale=id">Indonesia</a></li>
                  
                </ul>
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=it">Italiano</a></li>
                  
                      <li><a rel="nofollow" href="?locale=ja">日本語</a></li>
                  
                      <li><a rel="nofollow" href="?locale=nl">Nederlands</a></li>
                  
                      <li><a rel="nofollow" href="?locale=no">Norsk</a></li>
                  
                      <li><a rel="nofollow" href="?locale=pl">Polski</a></li>
                  
                </ul>
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=pt-BR">Português (BR)</a></li>
                  
                      <li><a rel="nofollow" href="?locale=ru">Русский</a></li>
                  
                      <li><a rel="nofollow" href="?locale=sr">Српски</a></li>
                  
                      <li><a rel="nofollow" href="?locale=sv">Svenska</a></li>
                  
                      <li><a rel="nofollow" href="?locale=zh">中文</a></li>
                  
                </ul>
              
            </div>

          </div>
          <div class="fade"></div>
        </div>
      
    

    <script>window._auth_token = "cf1c0b8959733617e26844d243ba9e11c32dfb89"</script>
    

<div id="keyboard_shortcuts_pane" style="display:none">
  <h2>Keyboard Shortcuts <small><a href="#" class="js-see-all-keyboard-shortcuts">(see all)</a></small></h2>

  <div class="columns threecols">
    <div class="column first">
      <h3>Site wide shortcuts</h3>
      <dl class="keyboard-mappings">
        <dt>s</dt>
        <dd>Focus site search</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>?</dt>
        <dd>Bring up this help dialog</dd>
      </dl>
    </div><!-- /.column.first -->

    <div class="column middle" style='display:none'>
      <h3>Commit list</h3>
      <dl class="keyboard-mappings">
        <dt>j</dt>
        <dd>Move selected down</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>k</dt>
        <dd>Move selected up</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>t</dt>
        <dd>Open tree</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>p</dt>
        <dd>Open parent</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>c <em>or</em> o <em>or</em> enter</dt>
        <dd>Open commit</dd>
      </dl>
    </div><!-- /.column.first -->

    <div class="column last" style='display:none'>
      <h3>Pull request list</h3>
      <dl class="keyboard-mappings">
        <dt>j</dt>
        <dd>Move selected down</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>k</dt>
        <dd>Move selected up</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>o <em>or</em> enter</dt>
        <dd>Open issue</dd>
      </dl>
    </div><!-- /.columns.last -->

  </div><!-- /.columns.equacols -->

  <div style='display:none'>
    <div class="rule"></div>

    <h3>Issues</h3>

    <div class="columns threecols">
      <div class="column first">
        <dl class="keyboard-mappings">
          <dt>j</dt>
          <dd>Move selected down</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>k</dt>
          <dd>Move selected up</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>x</dt>
          <dd>Toggle select target</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>o <em>or</em> enter</dt>
          <dd>Open issue</dd>
        </dl>
      </div><!-- /.column.first -->
      <div class="column middle">
        <dl class="keyboard-mappings">
          <dt>I</dt>
          <dd>Mark selected as read</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>U</dt>
          <dd>Mark selected as unread</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>e</dt>
          <dd>Close selected</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>y</dt>
          <dd>Remove selected from view</dd>
        </dl>
      </div><!-- /.column.middle -->
      <div class="column last">
        <dl class="keyboard-mappings">
          <dt>c</dt>
          <dd>Create issue</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>l</dt>
          <dd>Create label</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>i</dt>
          <dd>Back to inbox</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>u</dt>
          <dd>Back to issues</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>/</dt>
          <dd>Focus issues search</dd>
        </dl>
      </div>
    </div>
  </div>

  <div style='display:none'>
    <div class="rule"></div>

    <h3>Network Graph</h3>
    <div class="columns equacols">
      <div class="column first">
        <dl class="keyboard-mappings">
          <dt><span class="badmono">←</span> <em>or</em> h</dt>
          <dd>Scroll left</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt><span class="badmono">→</span> <em>or</em> l</dt>
          <dd>Scroll right</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt><span class="badmono">↑</span> <em>or</em> k</dt>
          <dd>Scroll up</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt><span class="badmono">↓</span> <em>or</em> j</dt>
          <dd>Scroll down</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>t</dt>
          <dd>Toggle visibility of head labels</dd>
        </dl>
      </div><!-- /.column.first -->
      <div class="column last">
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">←</span> <em>or</em> shift h</dt>
          <dd>Scroll all the way left</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">→</span> <em>or</em> shift l</dt>
          <dd>Scroll all the way right</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">↑</span> <em>or</em> shift k</dt>
          <dd>Scroll all the way up</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">↓</span> <em>or</em> shift j</dt>
          <dd>Scroll all the way down</dd>
        </dl>
      </div><!-- /.column.last -->
    </div>
  </div>

  <div >
    <div class="rule"></div>

    <h3>Source Code Browsing</h3>
    <div class="columns threecols">
      <div class="column first">
        <dl class="keyboard-mappings">
          <dt>t</dt>
          <dd>Activates the file finder</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>l</dt>
          <dd>Jump to line</dd>
        </dl>
      </div>
    </div>
  </div>

</div>
    

    <!--[if IE 8]>
    <script type="text/javascript" charset="utf-8">
      $(document.body).addClass("ie8")
    </script>
    <![endif]-->

    <!--[if IE 7]>
    <script type="text/javascript" charset="utf-8">
      $(document.body).addClass("ie7")
    </script>
    <![endif]-->

    
    <script type='text/javascript'></script>
    
  </body>
</html>

