#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <queue>
#include <unordered_map>
using namespace std;

struct nodo {
	int id;
	int atrib_act;	//De qué atributo se trata
	int valor_atrib;  //Valor que toma el atributo
	bool terminal = false;
	bool valoracion;	// + o - si es un nodo terminal (si no, se ignora)
};

double entropia(double p, double n) {
	double log_base2_p;
	double log_base2_n;

	//Tratar los 0s
	log_base2_p = p == 0 ? 0 : log2(p);
	log_base2_n = n == 0 ? 0 : log2(n);

	return (-p * log_base2_p) + (-n * log_base2_n);
}

//Devuelve ri * infor(pi, ni), donde i es valor_atributo del tipo del atributo que está en la posicion pos_atrib 
double info(const vector<vector<string>>& lista_ejemplos, string valor_atributo, int pos_atrib) {
	vector<vector<string>> rama;
	int num_pos = 0, num_neg = 0;

	for (vector<string> v : lista_ejemplos) {
		//Si es el valor del atributo, lo metemos en la rama
		if (v.at(pos_atrib) == valor_atributo) {
			rama.push_back(v);
		}
	}

	for (vector<string> v : rama) {
		if (v.back() == "si") num_pos++;
		else num_neg++;
	}

	const int N = lista_ejemplos.size();
	const double ri = (double) rama.size() / (double)N;
	
	double p = rama.size() == 0 ? 0: (double)num_pos / (double) rama.size();
	double n = rama.size() == 0 ? 0: (double)num_neg / (double) rama.size();

	return ri * entropia(p, n);
}

//Calcula el merito del atributo situado en pos_atrib
double merito(const vector<vector<string>>& lista_ejemplos, int pos_atrib, const vector<string>& valores_atributo) {
	double merito = 0;
	for (string valor : valores_atributo) {
		merito += info(lista_ejemplos, valor, pos_atrib);
	}
	return merito;
}

//Devuelve un iterador a la posicion en la que se encuentra el mejor merito
vector<pair<double, string>>::iterator masMerito(vector<pair<double, string>>& meritos) {
	if (meritos.empty()) return meritos.begin();

	double menor = meritos.at(0).first;
	int pos_menor = 0;

	for (int i = 0; i < meritos.size(); i++) {
		if (meritos.at(i).first < menor) {
			menor = meritos.at(i).first;
			pos_menor = i;
		}
	}

	vector<pair<double, string>>::iterator it = meritos.begin();
	for (int i = 0; i < pos_menor; i++) it++;

	return it;
}

//Devuelve la posicion del atributo nombreAtributo en valores_atrib
int pos(string nombreAtributo, const vector<pair<string, vector<string>>>& valores_atrib) {
	int res = -1;

	for (int i = 0; i < valores_atrib.size(); i++) {
		if (valores_atrib.at(i).first == nombreAtributo) res = i;
	}

	return res;
}

void id3(const vector<string> lista_atributos, const vector<vector<string>> lista_ejemplos, nodo& raiz,
	const vector<pair<string, vector<string>>>& valores_atrib,
	queue<nodo*> nodos_mem, int& id_nodo, unordered_map<int, vector<nodo*>>& mapa_hijos) {

	raiz.id = id_nodo;
	raiz.terminal = false; //Por defecto no es terminal
	id_nodo++;

	if (lista_ejemplos.empty()) return;

	bool todo_true = true, todo_false = true;
	for (vector<string> p : lista_ejemplos) {
		if (todo_true) todo_true = p.back() == "si";
		if (todo_false) todo_false = p.back() == "no";
	}

	if (todo_true) {
		raiz.valoracion = true;
		raiz.terminal = true;
		return;
	}
	else if (todo_false) {
		raiz.valoracion = false;
		raiz.terminal = true;
		return;
	}

	if (lista_atributos.empty()) return; //TODO error

	//Primera componente es el valor del merito, segunda componente la posicion del atributo en valores_atrib
	vector<pair<double, string>> meritos;

	//Recorremos todos los atributos menos el último (que es si / no) y calculamos su merito 
	for (int i = 0; i < lista_atributos.size() - 1; i++) {
		int posicion = pos(lista_atributos.at(i), valores_atrib);
		meritos.push_back({ merito(lista_ejemplos, posicion, valores_atrib.at(posicion).second), lista_atributos.at(i)});
	}

	auto it = masMerito(meritos);  //Encontrar el atributo mejor
	int pos_atrib = pos(it->second, valores_atrib);    //Guardar la posicion del atributo mejor (su posicion en valores_atrib)

	string nombre_atrib = valores_atrib.at(pos_atrib).first;
	vector<string> valores_atrib_mejor = valores_atrib.at(pos_atrib).second;

	//Construir la nueva lista_atributos sin el atributo mejor
	vector<string> nueva_lista_atributos;

	for (string s : lista_atributos) {
		if (s != nombre_atrib) nueva_lista_atributos.push_back(s);
	}

	for (int i = 0; i < valores_atrib_mejor.size(); i++) {

		string valor = valores_atrib_mejor.at(i);
		//Construir la nueva lista_ejemplos con los ejemplos que tengan el valor 'valor' del atributo pos_atrib
		vector<vector<string>> nueva_lista_ejemplos;

		for (vector<string> ejemplo : lista_ejemplos) {
			if (ejemplo.at(pos_atrib) == valor) nueva_lista_ejemplos.push_back(ejemplo);
		}

		nodo* n = (nodo*)malloc(sizeof(nodo));
		nodos_mem.push(n);

		n->atrib_act = pos_atrib;
		n->valor_atrib = i;

		//LLamada recursiva
		id3(nueva_lista_atributos, nueva_lista_ejemplos, *n, valores_atrib, nodos_mem, id_nodo, mapa_hijos);
		mapa_hijos[raiz.id].push_back(n);
	}
}

void printArbol(nodo& raiz, unordered_map<int, vector<nodo*>>& mapa_hijos, queue<nodo*> cola,
	const vector<pair<string, vector<string>>>& valores_atrib) {

	cola.push(&raiz);

	if (raiz.terminal) {
		cout << " ------------------------------------------ " << endl;

		while (!cola.empty()) {
			nodo* n = cola.front();
			cola.pop();

			//Encontrar la entrada de valores_atrib que corresponde al atributo
			if (n->atrib_act != -1) {  //La raiz vale -1
				pair<string, vector<string>> p = valores_atrib.at(n->atrib_act);

				cout << "Atributo: " << p.first << " valor: " << p.second.at(n->valor_atrib) << endl;
			}
		}

		string valoracion = raiz.valoracion ? "+" : "-";
		cout << " Resultado: " << valoracion << endl;
		cout << " ------------------------------------------ " << endl;
	}
	else {
		for (int i = 0; i < mapa_hijos[raiz.id].size(); i++) printArbol(*mapa_hijos[raiz.id].at(i), mapa_hijos, cola, valores_atrib);
	}
}

bool contains(vector<string>& v, string& s) {
	bool res = false;
	for (string aux : v) res = res ? true : s == aux;
	return res;
}

//Convertir a ejemplo con enteros indicando valores en valores_atrib
vector<int> aVectorEnteros(const vector<string> ejemplo_s, const vector<pair<string, vector<string>>>& valores_atrib) {
	vector<int> ejemplo;
	int i = 0;
	for (int i = 0; i < ejemplo_s.size(); i++) {
		vector<string> valores_atrib_actual = valores_atrib.at(i).second;
		string s = ejemplo_s.at(i);

		//Recorrer los valores buscando el que concide con s
		for (int j = 0; j < valores_atrib_actual.size(); j++) {
			if (valores_atrib_actual.at(j) == s) {
				ejemplo.push_back(j);
			}
		}
	}
	return ejemplo;
}

//Valorar un ejemplo dado
pair<bool, bool> valorarEjemplo(nodo& raiz, unordered_map<int, vector<nodo*>>& mapa_hijos,
	const vector<pair<string, vector<string>>>& valores_atrib, const vector<int> ejemplo) {

	//Si es terminal devolver valoracion del nodo
	if (raiz.terminal) return { true, raiz.valoracion};

	bool existe_regla = false;

	//Si no es terminal recorrer los hijos
	for (nodo* hijo : mapa_hijos.at(raiz.id)) {
		int atributo_actual = hijo->atrib_act;

		//Si valor del ejemplo para ese atributo corresponde al nodo, devoler llamada recursiva con hijo actual como raiz
		if (hijo->valor_atrib == ejemplo.at(atributo_actual)) {
			existe_regla = true;
			return valorarEjemplo(*hijo, mapa_hijos, valores_atrib, ejemplo);
		}
	}

	//Solo se llega a este punto si no hay una regla que se pueda aplicar. El primer false se ignorará.
	if (!existe_regla) return { false, existe_regla };
}

int main() {
	nodo raiz;
	raiz.atrib_act = -1; //La raiz no representa a ningun atributo
	unordered_map<int, vector<nodo*>> mapa_hijos;

	auto consola_buffer = cin.rdbuf();

	//Obtener lista de atributos
	ifstream in("AtributosJuego.txt");
	auto cinbuf = std::cin.rdbuf(in.rdbuf());

	vector<string> lista_atributos;
	char c;
	string ac;

	while (cin) {
		cin >> c;
		if (c != ',') ac.push_back(c);
		else {
			string s = ac.substr(0, ac.size());
			lista_atributos.push_back(s);
			ac = "";
		}
	}
	//El ultimo atributo no tiene coma detrás
	if (!ac.empty()) {
		string s = ac.substr(0, ac.size() - 1);
		lista_atributos.push_back(s);
	}

	//Obtener lista de ejemplos
	ifstream in2("Juego.txt");
	cinbuf = std::cin.rdbuf(in2.rdbuf());

	vector<vector<string>> lista_ejemplos;

	string linea;
	char a;
	while (cin) {
		vector<string> ejemplo;
		cin >> linea;
		auto it = linea.begin();

		ac.clear();
		while (it != linea.end()) {
			if (*it != ',') ac.push_back(*it);
			else {
				string s = ac.substr(0, ac.size());
				ejemplo.push_back(s);
				ac = "";
			}
			it++;
		}
		//El ultimo atributo no tiene coma detrás
		if (!ac.empty()) {
			string s = ac.substr(0, ac.size());
			ejemplo.push_back(s);
		}

		//Aniadir ejemplo a lista_ejemplos
		lista_ejemplos.push_back(ejemplo);
		cin.getline(&a, 1);
	}

	//Obtener todos los valores posibles de cada atributo
	vector < pair<string, vector<string>>> valores_atrib;

	//Iniciar la lista de atributos con el nombre de los atributos y un vector vacío
	for (int i = 0; i < lista_atributos.size(); i++) {
		vector<string> aux;
		valores_atrib.push_back({ lista_atributos.at(i), aux });
	}

	//Rellenar los vectores con los valores
	//Recorrer lista de ejemplos
	for (vector<string> ejemplo : lista_ejemplos) {

		//Recorrer el ejemplo
		for (int i = 0; i < ejemplo.size(); i++) {
			string valor = ejemplo.at(i);
			vector<string> valores_atrib_actual = valores_atrib.at(i).second;

			//Insertar el valor (no se insertan repetidos)
			if (!contains(valores_atrib_actual, valor)) valores_atrib_actual.push_back(valor);
			valores_atrib.at(i).second = valores_atrib_actual;
		}
	}

	//Calcular meritos
	//Primera componente es el valor del merito, segunda componente la posicion del atributo en valores_atrib
	vector<pair<double, int>> meritos;

	//Recorremos todos los atributos menos el último (que es si / no)
	for (int i = 0; i < lista_atributos.size() - 1; i++) {
		meritos.push_back({ merito(lista_ejemplos, i, valores_atrib.at(i).second), i });
	}

	cout << "Meritos iniciales -----------------------------------" << endl;

	for (pair<double, int> merito : meritos) {
		cout << "Atributo: " << valores_atrib.at(merito.second).first << endl;
		cout << "Merito: " << merito.first << endl;
	}

	queue<nodo*> nodos_memoria;
	int id_sig = 1;
	id3(lista_atributos, lista_ejemplos, raiz, valores_atrib, nodos_memoria, id_sig, mapa_hijos);

	//Borrar los nodos guardados en memoria
	while (!nodos_memoria.empty()) {
		delete nodos_memoria.front();
		nodos_memoria.pop();
	}

	queue<nodo*> cola;
	cout << endl;
	cout << "--------------------------------------" << endl;
	cout << "----------- REGLAS -------------------" << endl;
	cout << "--------------------------------------" << endl;
	cout << endl;

	printArbol(raiz, mapa_hijos, cola, valores_atrib);

	cout << "Valorar ejemplo: " << endl;

	//Redefinir el bufer
	cin.rdbuf(consola_buffer);
	bool fin = false;

	while (!fin) {
		string s;

		cout << "Poner FIN si se quiere terminar, otra cosa si se quiere seguir: ";
		cin >> s;
		fin = s == "FIN";
		if (fin) break;
		
		vector<string> ejemplo_introducido;
		bool generar = true;
		for (int i = 0; i < lista_atributos.size() - 1; i++) {
			cout << "Introduzca valor para " + lista_atributos.at(i) << endl;
			cin >> s;

			//comprobar que el valor introducido existe
			if (!contains(valores_atrib.at(i).second, s)) {
				cout << "Ese no es un atributo valido" << endl;
				generar = false;
				break;
			}
			ejemplo_introducido.push_back(s);
		}

		cout << "-----------------------------" << endl;

		if (generar) {
			pair<bool, bool> resultado = valorarEjemplo(raiz, mapa_hijos, valores_atrib, aVectorEnteros(ejemplo_introducido, valores_atrib));

			//Existe regla
			if (resultado.first) {
				string info = resultado.second ? "JUGAR" : "NO JUGAR";
				cout << info << endl;
			}
			//No existe regla
			else {
				cout << "No existe regla aplicable" << endl;
			}

			cout << "---------------------------------------" << endl;
		}
	}
	return 0;
}