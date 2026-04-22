#include <gui/screen1_screen/Screen1View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <string.h>

Screen1View::Screen1View()
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    // --- KANIT TESTİ ---
    // Ekran ilk açıldığında doğrudan bu yazıyı basıyoruz.
    // Eğer ekran açıldığında bu yazıyı görüyorsan, TouchGFX kodlarında HİÇBİR sorun yok demektir.
    Unicode::strncpy(textAreaEpcBuffer, "SISTEM BEKLIYOR", TEXTAREAEPC_SIZE);
    textAreaEpc.invalidate();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

/**
 * Presenter'dan gelen EPC bilgisini ekrana yansıtan ana fonksiyon.
 */
void Screen1View::updateEpcText(char* epc)
{
    // C tarafındaki pointer'ın boş gelme ihtimaline karşı güvenlik kontrolü
    if(epc != nullptr && strlen(epc) > 0)
    {
        // 1. Gelen metni TextArea belleğine kopyala
        Unicode::strncpy(textAreaEpcBuffer, epc, TEXTAREAEPC_SIZE);

        // 2. Ekranın o bölgesini yenile (Invalidate)
        textAreaEpc.invalidate();
    }
}
