# origami-tests

Testy do zadania origami.
W rozwiązaniu niedokładności powinny być rozstrzygane na korzyść punktu **na linii**
i na korzyść punkty **w figurze**.

W naszych rozwiązaniach $\varepsilon = 10^{-6}$ do porównywania z zerem.

Poza podziałem na wielkość testy dzielą się na dwa rodzaje.
1. Testy całkowicie losowe
2. Testy gdzie jest tylko jedna kartka, reszta to zagięcia. Pytania są zawsze o ostatnią kartkę.
W powyższych typach wszystkie testy zawierają **tylko liczby całkowite**.

Są też testy które jako tako sprawdzają floaty, mianowicie punkty są przesunięte o ten sam
wektor o niecałkowitych wartościach, a koordynaty w pytaniach są pomnożone przez
liczbę wymierną o małych liczniku i mianowniku. Ten typ testów dzieli się na takie same
podrodzaje jak wyżej i znajduje się w folderze `float_test`.

TODO:
Zrobić testy customowe
