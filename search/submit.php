<?php
include 'search.php';

$keywords = array ();
$query = $_GET ['q'];
$debug = $_GET ['debug'];
$sorting = $_GET ['sorting'];

if ($query != null) {
	$tok = strtok ( $query, " " );
	
	if ($debug === 'true')
		$debug = true;
	else
		$debug = false;
	
	while ( $tok !== false ) {
		// Remove punctuation from the token
		$word = preg_replace ( "/[^a-zA-Z 0-9]+/", " ", $tok );
		
		$escaped = htmlspecialchars ( $word, ENT_QUOTES, 'UTF-8' );
		array_push ( $keywords, $escaped );
		
		$tok = strtok ( " " );
	}
	$ret = performSQLQuery ( $debug, $keywords, $sorting );
}

/* HTML search form */
echo '<div style="margin-left:130px;">';
echo '<form action="submit.php">';
echo '<a href=".."><img src="../images/thumb.png" alt="Widdershins" align="top"></img></a>';
echo '<input type="text" name="q" value="' . $query . '"> ';
echo '<select name="sorting">
	<option value="none">None</option>
  	<option value="name">Name</option>
	<option value="time">Time</option>
	</select>';
echo '<input value="Widdershins Search" name="search" type="submit">';
echo '<input type="checkbox" name="debug" value="true"> Check
	to enable debug mode';
echo '</form>';
echo '</div>';
echo '<hr>';

/* Search results */
echo $ret;

?>
	