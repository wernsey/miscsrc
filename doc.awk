BEGIN { 
	if(!title) title = "Documentation"
	print "<html><head><title>" title "</title>";
	print "<style><!--";
	print "tt,b {color:rgb(64,128,64)}";
	print "pre {background:rgb(245,255,245);color:rgb(64,128,64);margin-left:30px;margin-right:30px}";
	print "h2 {background:rgb(230,255,230);text-align:center}";
	print "h3 {font-family:monospace;color:rgb(64,128,64);background:rgb(240,255,240)}";
	print "body {font-family:Arial, Verdana, Helvetica, sans-serif;margin-left:20px;margin-right:20px}";
	print "--></style>";
	print "</head><body>";
}

/\/\*/ { comment = 1; }

/\*!/ { if(!comment) next; s = substr($0, index($0, "*!") + 2); print "<h2><tt>" filter(s) "</tt></h2>"; next;}
/\*@/ { if(!comment) next; s = substr($0, index($0, "*@") + 2); print "<h3><tt>" filter(s) "</tt></h3>"; next;}
/\*#[ \t]*$/ { if(!comment) next; if(!pre) print "<br>"; next;}
/\*#/ { if(!comment) next; s = substr($0, index($0, "*#") + 2); print filter(s);}
/\*&/ { if(!comment) next; s = substr($0, index($0, "*&") + 2); print "<tt>" filter(s) "</tt><br>"; next;}
/\*x/ { if(!comment) next; s = substr($0, index($0, "*x") + 2); print "<br><b>Example:</b><tt>" filter(s) "</tt>"; next;}
/\*\[/ { if(!comment) next; pre=1; print "<pre>"; next;}
/\*]/ { if(!comment) next; pre=0; print "</pre>"; next;}
/\*\{/ { if(!comment) next; print "<ul>"; next;}
/\*\*/ { if(!comment) next; s = substr($0, index($0, "**") + 2); print "<li>" filter(s); next;}
/\*}/ { if(!comment) next; print "</ul>"; next;}
/\*-/ { if(!comment) next; print "<hr size=2>"; next;}
/\*=/ { if(!comment) next; print "<hr size=5>"; next;}
/\*N/ { if(!comment) next; s = substr($0, index($0, "*N") + 2); print "<br><b>Note:</b><i>" filter(s) "</i><br>"; next;}

/\*\// { comment = 0; }

END { print "</body></html>" }

function filter(ss,        j, k1, k2, k3)
{
	gsub(/&/, "\\&amp;", ss); 
	gsub(/</, "\\&lt;", ss); 
	gsub(/>/, "\\&gt;", ss);
	gsub(/\\n[ \t]*$/, "<br>", ss);
	gsub(/{{/, "<tt>", ss); 
	gsub(/}}/, "</tt>", ss);
	gsub(/{\*/, "<b>", ss); 
	gsub(/\*}/, "</b>", ss);
	gsub(/{\//, "<i>", ss); 
	gsub(/\/}/, "</i>", ss);
	gsub(/{_/, "<u>", ss); 
	gsub(/_}/, "</u>", ss);	
	
	# Hyperlinks (excuse my primitive regex)
	gsub(/http:\/\/[a-zA-Z0-9._\/-]+/, "<a href=\"&\">&</a>", ss);
	
	# Use a ##word to specify an anchor, eg. ##foo gets translated to <a name="foo">foo</a>
	while(j = match(ss, /##[A-Za-z0-9_]+/))
	{
		k1 = substr(ss, 1, j - 1);
		k2 = substr(ss, j + 2, RLENGTH-2);
		k3 = substr(ss, j + RLENGTH);
		ss = k1 "<a name=\"" k2 "\">" k2 "</a>" k3
	}
	
	# Use a ~~word to specify an anchor, eg. ~~foo gets translated to <a href="#foo">foo</a>
	while(j = match(ss, /~~[A-Za-z0-9_]+/))
	{
		k1 = substr(ss, 1, j - 1);
		k2 = substr(ss, j + 2, RLENGTH-2);
		k3 = substr(ss, j + RLENGTH);
		ss = k1 "<a href=\"#" k2 "\">" k2 "</a>" k3
	}
	
	gsub(/\*\//, "", ss);
	return ss;
}