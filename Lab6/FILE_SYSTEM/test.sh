#!/bin/sh

FILENAME='VFS'

echo 'Utworzenie dysku o rozmiarze 10240 bajtow. 4 inody + 4 bloki danych + 1 superblok:'
./a.out $FILENAME create 10240
echo 'Drukujemy na ekran zrzut danych'
./a.out $FILENAME info

echo 'Zapis pliku o rozmiarze 2kb. Powinien zajac dokladnie jeden, pierwszy blok'
./a.out $FILENAME add 2kb.txt 2kb
echo 'Wyswietlenie wyniku'
./a.out $FILENAME info
echo 'Zapis pliku o rozmiarze 2kb + 1.'
./a.out $FILENAME add 2k1b.txt 2k1b
echo 'Wyswietlamy stan'
./a.out $FILENAME info

echo 'Wyswietlamy liste plikow'
./a.out $FILENAME ls

echo 'Proba dodania pliku o rozmiarze 2kb + 1 skonczy sie bledem - za malo miejsca'
./a.out $FILENAME add 2k1b.txt 2k1b_2

echo 'Proba dodania pliku o rozmiarze jednego bloku, ale o nazwie 2kb - blad - plik o tej nazwie juz istnieje.'
./a.out $FILENAME add 2kb.txt 2kb

echo 'Zabezpieczenie przed fragmentacja. Usuniecie plik 2kb i dodanie drugiego pliku o rozmiarze 2kb + 1.'
./a.out $FILENAME remove 2kb
./a.out $FILENAME add 2k1b.txt 2k1b_2

echo 'Wynik'
./a.out $FILENAME info

echo 'Usuniecie srodkowego pliku i wyciagniecie pliku z systemu wirtualnego.'
./a.out $FILENAME remove 2k1b

./a.out $FILENAME copy 2k1b_2 2k1b_2.copied

echo 'Wynik'
./a.out $FILENAME info

echo 'Dodanie pliku 1025 bajtow:'
./a.out $FILENAME add 1k1b.txt 1k1b
echo 'Wynik'
./a.out $FILENAME info
