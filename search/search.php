<?php

/**
 * This file contains all of the functions regarding search queries
 */

/**
 * This function must be called before any queries
 */
function initMySQL() {
	$db = new PDO ( 'mysql:host=localhost;dbname=Search;charset=utf8mb4', 'user', 'password' );
	if (! $db) {
		echo 'error';
		return null;
	}
	return $db;
}

/**
 * Searches the SQL database for keywords.
 * If words are seperated with a space, it searches for rows that contain
 * both words.
 */
function performSQLQuery($debug, $keywords, $sorting) {
	$con = mysql_connect ( "localhost:80", "user", "password" ) or die ( "Could not connect: " . mysql_error () );
	mysql_select_db ( "Search" );
	$str = '';
	
	$query = 'SELECT url, title, description FROM url_list WHERE data LIKE ';
	
	/*
	 * queries with multiple words have data which contains all words
	 */
	for($i = 0; $i < count ( $keywords ); $i ++) {
		$word = '%' . ( string ) $keywords [$i] . '%'; // Add wildcards
		$query = $query . '"' . $word . '"';
		
		/* Non edge cases */
		if ($i !== count ( $keywords ) - 1)
			$query = $query . ' AND data LIKE ';
	}
	
	/* Add sorting options, if applicable */
	if ($sorting === 'name') {
		$query = $query . ' ORDER BY title';
	} else if ($sorting == 'time') {
		$query = $query . ' ORDER BY id DESC';
	}
	$query = $query . ';';
	
	if ($debug) {
		echo '<h4>Debug info:</h4>';
		if ($sorting != null) {
			echo 'Sorting option: ' . $sorting;
		} else
			echo 'No sorting option provided';
		
		echo '<br>';
		echo 'MySQL Query: ' . $query;
		echo '<hr>';
	}
	
	$result = mysql_query ( $query ) or die ( mysql_error () );
	$num_results = mysql_num_rows ( $result );
	
	$str = $str . '<div style="margin-left:130px;margin-bottom:30px;color:#808080;">';
	$str = $str . 'About ' . $num_results . ' results';
	$str = $str . '</div>';
	
	while ( $row = mysql_fetch_assoc ( $result ) ) {
		if (strpos ( $row ['title'], '404' ) !== false)
			continue;
		if (strpos ( $row ['title'], '301' ) !== false)
			continue;
		if (strpos ( $row ['title'], '302' ) !== false)
			continue;
		if ($row ['title'] === 'The title for this webpage could not be displayed.')
			continue;
		
		$str = $str . '<div style="margin:0 auto;">';
		$str = $str . '<div style="margin-left:130px;margin-bottom:0;margin-top:0;">';
		
		$str = $str . '<h3 style="margin-bottom:0;margin-top:0">';
		$str = $str . '<a href="' . $row ['url'] . '">';
		$str = $str . $row ['title'];
		$str = $str . '</a>';
		/*
		 * Finish up link
		 */
		$str = $str . '</h3>';
		
		/* Cite url */
		$str = $str . '<p style="color:#006621;margin-top:0;margin-bottom:0;">';
		$str = $str . $row ['url'] . '</p>';
		
		/* Description */
		$str = $str . '<p style="margin-top:0;margin-bottom:0;">';
		$str = $str . $row ['description'];
		$str = $str . '</p>';
		
		$str = $str . '</div>';
		$str = $str . '</div>';
		$str = $str . '<br style="margin-bottom:30px;">';
	}
	
	// $str = $str . '</table>';
	
	return $str;
}
?>