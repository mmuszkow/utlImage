/* 2011 adrian_007
 * plik z definicja struktury dzieki ktorej mozemy przekazac wtyczce informacje o
 * pliku ktory chcemy wyslac na serwer imageshack.us (sciezka musi byc pelna)
 * przyjmuje jpg/jpeg, png, gif
*/

#ifndef WTW_IMAGESHACK_EXTERNAL_H
#define WTW_IMAGESHACK_EXTERNAL_H
#ifdef _WIN32

typedef struct {
	const wchar_t*	filePath;		// sciezka do pliku ktory chcemy wyslac

	const wchar_t*	contactId;		// id kontaktu do ktorego chcemy wyslac link do danego pliku
	const wchar_t*	netClass;		// siec
	int				netId;			// identyfikator sieci
}wtwImagesHackFileV1;

typedef wtwImagesHackFileV1 wtwImagesHackFile;

#define WTW_IMAGESHACK_SEND_IMAGE L"wtwImageShack/sendImage"

#endif // _WIN32
#endif // WTW_IMAGESHACK_EXTERNAL_H
