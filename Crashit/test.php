<?php
	if(!extension_loaded('crashit')) {
		dl('crashit.' . PHP_SHLIB_SUFFIX);
	}

	if(!extension_loaded('crashit')) {
		die("crashit extension is not available, please compile it.\n");
	}

	av();
	//endapis(5);

?>
