// Symulator maszyny Turinga
// Autor: Leszek Godlewski <leszgod081@student.polsl.pl>
// Kod źródłowy udostępniony na licencji WTFPL. :)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// prosty typ logiczny
typedef unsigned char	bool_t;
#define	bfalse			0
#define btrue			!bfalse

// typy do konstrukcji tabeli charakterystycznej
typedef struct {
	short				sym;	// symbol do zapisania na aktualnej pozycji glowicy; wszystko poza
								// przedzialem [0; 255] traktujemy jako pusty symbol
	signed char			move;	// wartosci < 0 przesuwaja glowice w lewo, 0 zostawiaja
								// na miejscu, > 0 przesuwaja w prawo
	int					state;	// indeks stanu, do ktorego przechodzimy
} instruction_t;

typedef struct {
	short				sym;	// symbol, ktorego dotyczy dany wiersz; wszystko poza
								// przedzialem [0; 255] traktujemy jako pusty symbol
	instruction_t		*state;	// tablica stanow dla danego symbolu
} table_row_t;

// typ do konstrukcji tasmy jako linkowanej listy
typedef struct tape_elem_s {
	short				sym;	// symbol na danym miejscu tasmy; wszystko poza
								// przedzialem [0; 255] traktujemy jako pusty symbol
	struct tape_elem_s	*prev;
	struct tape_elem_s	*next;
} tape_element_t;

// typ glowicy maszyny
typedef struct {
	tape_element_t		*pos;	// symbol, na ktorym obecnie znajduje sie glowica
	size_t				state;	// indeks stanu wewn., w ktorym jest glowica
} head_t;

// opcje czytane z linii polecen
bool_t	debug			= bfalse;
bool_t	step			= bfalse;
bool_t	tail			= bfalse;

// tablica charakterystyczna
table_row_t				*table;
size_t					num_table_rows = 0;
size_t					num_table_states = 0;

// tasma
tape_element_t			*tape_head = NULL;	// wskaznik do elementu najbardziej po lewej
tape_element_t			*tape_tail = NULL;

// glowica
head_t					head;

// pokazuje pomoc w uzyciu programu
void print_help(const char *argv0) {
	printf("Użycie: %s [opcje] <tablica> <taśma>\n"
			"Opcje:\n"
			"  -t, --tail   powoduje zainicjowanie ustawienia głowicy w taki sposób, by była\n"
			"               ustawiona za prawym końcem napisu, w przeciwieństwie do zachowania\n"
			"               domyślnego, kiedy jest ustawiana z lewej jego strony\n"
			"  -g, --debug  wypisuje wszystkie kroki działania maszyny przy wykonywaniu\n"
			"               programu\n"
			"  -s, --step   powoduje wejście w tryb interaktywny, w którym przejście do\n"
			"               następnego kroku następuje po naciśnięciu klawisza przez\n"
			"               użytkownika (implikuje --debug)\n"
			"\n"
			"Autor: Leszek Godlewski <leszgod081@student.polsl.pl>\n", argv0);
}

// parsuje argumenty przekazane z linii polecen i ustawia odpowiednio opcje
bool_t parse_args(int argc, char *argv[], char **program, char **tape) {
	int i;

	if (argc < 3)
		return bfalse;

	// najpierw parsujemy opcje; jesli cos nam nie pasuje, zwracamy bfalse
	// i pokazujemy instrukcje uzycia
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'g':
					debug = btrue;
					break;
				case 's':
					step = btrue;
					break;
				case 't':
					tail = btrue;
					break;
				case '-':
					if (!strcmp(argv[i] + 2, "debug"))
						debug = btrue;
					else if (!strcmp(argv[i] + 2, "step"))
						step = btrue;
					else if (!strcmp(argv[i] + 2, "tail"))
						tail = btrue;
					else
						return bfalse;
				default:
					return bfalse;
			}
		} else
			break;
	}

	// jesli nie zostaly nam dokladnie 2 argumenty, uzytkownik podal je zle
	if (argc - i != 2)
		return bfalse;

	// pliki do otwarcia
	*program = argv[i];
	*tape = argv[i + 1];

	// udalo sie przeparsowac argumenty
	return btrue;
}

// funkcja pomocnicza - czyta pojedynczy token z pliku CSV
void read_token(FILE *f, char *token) {
	int c;
	char *p = token;
	while (1) {
		c = fgetc(f);
		if (c == ',' || c == '\n')
			break;
		// usuwanie ewentualnych cudzyslowow
		if (p == token && c == '\"')
			continue;
		*p++ = c;
	}
	// usuwanie ewentualnych cudzyslowow
	if (p > token + 1 && *(p - 1) == '\"' && *(p - 2) != '\\')
		*(p - 1) = 0;
	else
		*p = 0;
}

// czyta tablice charakterystyczna z pliku
bool_t program_load(const char *fname) {
	char	token[32], buf[2];
	int		c;	// znak zczytany z pliku
	int		i, j, k;
	FILE	*f = fopen(fname, "rb");

	if (!f)
		return bfalse;

	// najpierw liczymy ilosc kolumn i wierszy tablicy
	k = 0;	// licznik przecinkow
	while ((c = fgetc(f)) != EOF) {
		if (c == ',')
			k++;
		else if (c == '\n') {
			if (k > num_table_states)
				num_table_states = k;
			k = 0;
			num_table_rows++;
		}
	}
	// ostatni wiersz moze nie konczyc sie znakiem nowej linii
	if (k > 0)
		num_table_rows++;

	// przydzielamy pamiec na tablice i czyscimy ja
	table = malloc(sizeof(table_row_t) * num_table_rows);
	memset(table, 0, sizeof(table_row_t) * num_table_rows);
	for (i = 0; i < num_table_rows; i++) {
		table[i].state = malloc(sizeof(instruction_t) * num_table_states);
		memset(table[i].state, 0, sizeof(instruction_t) * num_table_states);
		for (j = 0; j < num_table_states; j++)
			// inicjujemy do -1 zeby wykrywac bledy
			table[i].state[j].state = -1;
	}

	// wracamy na poczatek pliku i zaczynamy wlasciwe parsowanie
	rewind(f);
	for (i = 0; i < num_table_rows; i++) {
		read_token(f, token);

		if (token[0] == '\\' && token[1] == 'q')
			// symbol pusty, oznaczamy przez -1
			table[i].sym = -1;
		else {
			// FIXME: to niezbyt bezpieczne, ale nie chce mi sie kodowac escape sequences
			snprintf(buf, sizeof(buf), token);
			table[i].sym = buf[0];
		}

		for (j = 0; j < num_table_states; j++) {
			read_token(f, token);

			// parsujemy od tylu - najpierw numer docelowego stanu
			for (k = strlen(token) - 1; k >= 0 && token[k] != 'q'; k--) {}
			if (token[k] == 'q') {
				token[k] = 0;
				table[i].state[j].state = strtol(token + k + 1, NULL, 10) - 1;
			}
			if (table[i].state[j].state < 0 || table[i].state[j].state >= num_table_states) {
				printf("Błąd składni pliku tablicy charakterystycznej - nieprawidłowy stan.\n");
				return bfalse;
			}

			// kierunek ruchu
			k--;
			if (k > 0) {
				switch (token[k]) {
					case 'L':
						table[i].state[j].move = -1;
						break;
					case 'N':
						table[i].state[j].move = 0;
						break;
					case 'R':
					case 'P':
						table[i].state[j].move = 1;
						break;
					default:
						printf("Błąd składni pliku tablicy charakterystycznej - nieprawidłowy ruch.\n");
						return bfalse;
				}
			} else {
				printf("Błąd składni pliku tablicy charakterystycznej - prawdopodobnie brak symbolu\n"
						"w instrukcji.\n");
				return bfalse;
			}

			// symbol
			token[k] = 0;
			if (token[0] == '\\' && token[1] == 'q')
				// symbol pusty, oznaczamy przez -1
				table[i].state[j].sym = -1;
			else {
				// FIXME: to niezbyt bezpieczne, ale nie chce mi sie kodowac escape sequences
				snprintf(buf, sizeof(buf), token);
				table[i].state[j].sym = buf[0];
			}
		}
	}

	fclose(f);

	return btrue;
}

// dodaje nowy symbol na lewym koncu tasmy
void tape_prepend(short symbol) {
	tape_element_t *e = malloc(sizeof(tape_element_t));
	memset(e, 0, sizeof(*e));
	e->sym = symbol;
	if (tape_head) {
		tape_head->prev = e;
		e->next = tape_head;
	} else if (!tape_tail)
		tape_tail = e;
	tape_head = e;
}

// dodaje nowy symbol na prawym koncu tasmy
void tape_append(short symbol) {
	tape_element_t *e = malloc(sizeof(tape_element_t));
	memset(e, 0, sizeof(*e));
	e->sym = symbol;
	if (tape_tail) {
		tape_tail->next = e;
		e->prev = tape_tail;
	} else if (!tape_head)
		tape_head = e;
	tape_tail = e;
}

// "przycina" tasme - zwalnia puste symbole po obu koncach
void tape_trim() {
	tape_element_t	*e;
	for (e = tape_head; e; e = e->next) {
		if (e->sym < 0 || e->sym > 256) {
			if (e->next) {
				tape_head = e->next;
				tape_head->prev = NULL;
			} else
				tape_head = tape_tail = NULL;
			free(e);
		} else
			break;
	}
	for (e = tape_tail; e; e = e->prev) {
		if (e->sym < 0 || e->sym > 256) {
			if (e->prev) {
				tape_tail = e->prev;
				tape_tail->next = NULL;
			} else
				tape_head = tape_tail = NULL;
			free(e);
		} else
			break;
	}
}

// wypisuje zawartosc tasmy
void tape_print() {
	tape_element_t	*e;
	for (e = tape_head; e; e = e->next) {
		if (e->sym >= 0 && e->sym < 256)
			fputc(e->sym, stdout);
		else
			fputs("\\q", stdout);
	}
	fputc('\n', stdout);
}

// czyta tasme z pliku
bool_t tape_load(const char *fname) {
	int		c;
	FILE	*f = fopen(fname, "rb");

	if (!f)
		return bfalse;

	while ((c = fgetc(f)) != EOF) {
		if (c == '\\') {
			if ((c = fgetc(f)) == 'q')
				tape_append(-1);
			else {
				tape_append('\\');
				if (c != EOF)
					tape_append(c);
			}
		} else
			tape_append(c);
	}

	return btrue;
}

// przesuwa glowice w zadanym kierunku
void head_move(signed char dir) {
	if (dir < 0) {
		if (!head.pos->prev) {
			assert(head.pos == tape_head);
			tape_prepend(-1);
		}
		head.pos = head.pos->prev;
	} else if (dir > 0) {
		if (!head.pos->next) {
			assert(head.pos == tape_tail);
			tape_append(-1);
		}
		head.pos = head.pos->next;
	} // w przeciwnym wypadku nic nie rob
}

// funkcja pomocnicza - znajduje wiersz, ktory odpowiada danemu symbolowi
int locate_row(short symbol) {
	int i;
	for (i = 0; i < num_table_rows; i++) {
		if ((symbol < 0 || symbol > 255)
			&& (table[i].sym < 0 || table[i].sym > 255))
			break;
		if (symbol == table[i].sym)
			break;
	}
	if (i >= num_table_rows) {
		printf("BŁĄD: Brak symbolu %c w tabeli charakterystycznej!\n", (char)symbol);
		exit(1);
	}
	return i;
}

// glowna funkcja wykonujaca zadnie maszyny
void machine_run(void) {
	int	i = locate_row(head.pos->sym);
	while (1) {
		// znajdujemy odpowiedni wiersz tabeli

		// wypisz obecny stan maszyny i instrukcje, ktora bedziemy wykonywac
		if (debug || step)
			printf("%c%c q%d -> [%c%c %c q%d] -> ",
				(head.pos->sym >= 0 && head.pos->sym < 256)
					? ' ' : '\\',
				(head.pos->sym >= 0 && head.pos->sym < 256)
					? (char)head.pos->sym : 'q',
				(head.state + 1),
				(table[i].state[head.state].sym >= 0
					&& table[i].state[head.state].sym < 256)
					? ' ' : '\\',
				(table[i].state[head.state].sym >= 0
					&& table[i].state[head.state].sym < 256)
					? (char)table[i].state[head.state].sym : 'q',
				(table[i].state[head.state].move > 0) ? 'P' :
					((table[i].state[head.state].move < 0) ? 'L' : 'N'),
				table[i].state[head.state].state + 1);

		// wykonaj instrukcje
		head.pos->sym = table[i].state[head.state].sym;
		head.state = table[i].state[head.state].state;
		head_move(table[i].state[head.state].move);

		// wypisz nowy stan maszyny
		if (debug || step)
			printf("%c%c q%d\n",
				(head.pos->sym >= 0 && head.pos->sym < 256)
					? ' ' : '\\',
				(head.pos->sym >= 0 && head.pos->sym < 256)
					? (char)head.pos->sym : 'q',
				head.state + 1);

		i = locate_row(head.pos->sym);

		// sprawdz, czy nie wpadamy w nieskonczona petle (oczywiste przypadki):
		// 1) nie poruszamy sie i nie zmieniamy stanu...
		if ((head.pos->sym == table[i].state[head.state].sym
				&& head.state == table[i].state[head.state].state
				&& table[i].state[head.state].move == 0)
			// ...albo 2) nie zmieniamy stanu, mamy pusty symbol i "odjezdzamy"
			// w dal od napisu
			|| (head.state == table[i].state[head.state].state
				&& (head.pos->sym < 0 || head.pos->sym > 255)
				&& ((head.pos == tape_head
						&& table[i].state[head.state].move < 0)
					|| ((head.pos == tape_tail
						&& table[i].state[head.state].move > 0))))) {
			printf("\nAutomat znalazł się w nieskończonej pętli.\n");
			break;
		}

		if (step)
			fgetc(stdin);
	}
}

// zwalnianie pamieci
void cleanup(void) {
	int				i;
	tape_element_t	*e;
	if (table) {
		for (i = 0; i < num_table_rows; i++) {
			if (table[i].state)
				free(table[i].state);
		}
		free(table);
	}
	e = tape_head;
	while (e) {
		if (e->next) {
			e = e->next;
			free(e->prev);
		} else {
			free(e);
			e = NULL;
		}
	}
}

int main(int argc, char *argv[]) {
	char	*program	= NULL;
	char	*tape		= NULL;

    printf("Symulator maszyny Turinga [" __DATE__ "]\n");

    // przeparsuj argumenty programu; jesli uzytkownik sie pomylil, pokaz pomoc i wyjdz
    if (!parse_args(argc, argv, &program, &tape)) {
    	print_help(argv[0]);
    	return 0;
    }

	// zaladuj dane wejsciowe
	if (!program_load(program)) {
		cleanup();
		printf("Błąd przy otwieraniu pliku tablicy charakterystycznej %s!", program);
		return 1;
	}
	if (!tape_load(tape)) {
		cleanup();
		printf("Błąd przy otwieraniu pliku taśmy %s!", tape);
		return 1;
	}

	printf("Stan taśmy przed uruchomieniem maszyny:\n");
	tape_print();

	// zainicjuj glowice
	memset(&head, 0, sizeof(head));
	if (tail) {
		head.pos = tape_tail;
		head_move(1);
	} else {
		head.pos = tape_head;
		head_move(-1);
	}

	// uruchom maszyne
	machine_run();

	// wypisz nowy stan tasmy
	tape_trim();
	printf("Stan taśmy po zakończeniu działania maszyny:\n");
	tape_print();

	cleanup();

    return 0;
}
