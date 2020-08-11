/////////////////////////////
// tinyweb.js  (C) 07/2020 NET234 Pierre HENRY  
// https://github.com/net234/MiniServeurWeb
//
// Recuperation des données dynamique via une HTTP POST sur la page d'origine
// Fonctionne avec la librairie tinyweb.h
// 
// V1.0 
// 
// Global
var req_data = null;
var refreshTimeout = 0;

// GetAsyncData envoie une requête a l'adresse courante 
function GetAsyncData() {
	var refreshList = document.querySelectorAll('.refresh');
	if (refreshList.length == 0) {
		return;  // abort   refreshList will not be call again
	}
	
	var url = window.location;
	req_data = null;
	// section pour les objets XMLHttpRequest natifs
	if (window.XMLHttpRequest) {
		req_data = new XMLHttpRequest();
		// 3 lignes pour windaube
	} else if (window.ActiveXObject) {
		req_data = new ActiveXObject('Microsoft.XMLHTTP');
	}
	if (req_data == null) {
		return;  //abort no XMLHttpRequest allowed here
	}
	req_data.abort();
	req_data.onreadystatechange = GotAsyncData;
	req_data.open('POST', url, true);
	req_data.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");

	var refreshString = 'refresh='+ refreshTimeout;
	for (var item of refreshList) {
		if (item.innerHTML.length <= 100) {
			refreshString += '&' + item.id + '=' + encodeURIComponent(item.innerHTML);
		} else {
			refreshString += '&' + item.id + '_TOO_BIG'
		}
	}
	req_data.send(refreshString);
}

// GotAsyncData est le callback de lecture pour l'appel de XMLHttpRequest() ci-desssus.
// Cette procédure n'est executée que si des données sont obtenues par la requête.

function GotAsyncData() {
	// seulement si req_data affiche "loaded"
	if (req_data.readyState != 4 || req_data.status != 200) {
		return;
	}
	// clear unwanted answer from standard server (generaly we got the full html page)
	if (req_data.responseText.length > 1000) {
		return; //Abort refresh  no timeout
	}
	// parse pair key/value and put them back in place with size limitation
	var urlParams = new URLSearchParams(req_data.responseText);
	for (var item of urlParams) {
		// Internal value refreshTimeout can be changed (or displayed too)
		if (item[0] == 'refresh') {
			refreshTimeout = item[1].valueOf();
		}
		// Document value changed with 100Byte limitation
		var aEle = document.getElementById(item[0]);
		if (aEle) {
			if (item[1].length <= 200) {
				aEle.innerHTML = item[1].trim();
			} else {
				// todo:  cut the size or warn user ?
				aEle.innerHTML = item[0] + '__TOO_BIG';
			}
		}
	}
	try {
      // callback to user js code to warn abour end of refresh
	  refreshPage();
	} catch {		
	}
	// Next iteration
	if (refreshTimeout > 0 ) {
		if (refreshTimeout < 100) {
			refreshTimeout = 100
		}
		setTimeout("GetAsyncData()", refreshTimeout);
	}
	return;
}
