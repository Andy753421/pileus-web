function show() {
	var hidden = document.getElementsByClassName('hidden');
	var shown  = document.getElementsByClassName('shown');
	for (i = 0; i < hidden.length; i ++)
		hidden[i].style.display = "block";
	for (i = 0; i < shown.length; i ++)
		shown[i].style.display = "none";
}
