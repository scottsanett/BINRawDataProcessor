#include <QCoreApplication>
#include <QString>
#include <QVector>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDebug>

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <array>
#include <map>
#include <utility>

using entry_t = std::array<QString, 6>;
using matrix_t = QVector<entry_t>;
using matcol_t = QVector<matrix_t>;
using rules_t = std::map<QString, QString>;

bool loadInformation(std::string const &, matrix_t & mat);
bool translateCategory(matrix_t & mat, rules_t & categories);
bool translateMarks(matrix_t & mat, rules_t marks);
void split(matrix_t const & mat, matcol_t & col, int num);
void outputSource( matcol_t const & col, int num, std::string const & fileName);
void outputSourceIndex(matcol_t const & col, int num, std::string const & fileName);
void outputSourceReverseIndex(matcol_t const & col, int num, std::string const & fileName);

int main(int argc, char *argv[])
{
    matrix_t mat;
    rules_t categories, marks;
    std::string fileName, nameOfOutput;
    int numOfFiles;

    std::cout << "Input name of file to be processed: ";
    std::getline(std::cin, fileName);

    std::cout << "Input name of output file: ";
    std::getline(std::cin, nameOfOutput);

    std::cout << "Input number of files you wish to split into: ";
    std::cin >> numOfFiles;

    if (!loadInformation(fileName, mat)) return -1;
    if (!translateCategory(mat, categories)) return -1;
    if (!translateMarks(mat, marks)) return -1;

    matcol_t collection;
    collection.fill(matrix_t{}, numOfFiles);
    split(mat, collection, numOfFiles);
    outputSource(collection, numOfFiles, nameOfOutput);
    outputSourceIndex(collection, numOfFiles, nameOfOutput);
    outputSourceReverseIndex(collection, numOfFiles, nameOfOutput);
}

bool loadInformation(std::string const & fileName, matrix_t & mat) {
    std::string line;
    if (fileName.empty()) { return false; }
    std::ifstream ifs(fileName);
    while (std::getline(ifs, line)) {
        auto entries = QString(line.c_str()).split(';');
        auto lemma = entries.at(0);
        auto index = entries.at(1);
        auto category = entries.at(2);
        auto unknown = entries.at(3);
        auto form = entries.at(4);
        auto mark = entries.at(5);
        entry_t entry;
        entry[0] = lemma;
        entry[1] = index;
        entry[2] = category;
        entry[3] = unknown;
        entry[4] = form;
        entry[5] = mark;
        mat.push_back(entry);
    }
    return true;
}

bool translateCategory(matrix_t & mat, rules_t & categories) {
    std::string line;
    QFile file(":/rules/cat.txt");
    if(!file.open(QIODevice::ReadOnly)) {
        std::cout << "Error: can't open cat.txt" << std::endl;
        return false;
    }
    QTextStream qts(&file);
    while (!qts.atEnd()) {
        QString qline = qts.readLine();
        line = qline.toStdString();
        auto pos = line.find_first_of(' ');
        auto firstHalf = line.substr(0, pos);
        auto secondHalf = line.substr(pos + 1, line.length() - firstHalf.length() - 1);
        categories.insert(std::make_pair(firstHalf.c_str(), secondHalf.c_str()));
    }

    for (auto && i : mat) {
        auto before = i.at(2);
        auto after = categories[before];
        i.at(2) = after;
    }
    return true;
}

bool translateMarks(matrix_t & mat, rules_t marks) {
    std::string line;
    QFile file(":/rules/rules.txt");
    if(!file.open(QIODevice::ReadOnly)) {
        std::cout << "Error: can't open cat.txt" << std::endl;
        return false;
    }
    QTextStream qts(&file);
    while (!qts.atEnd()) {
        QString qline = qts.readLine();
        line = qline.toStdString();
        auto pos = line.find_first_of(' ');
        auto firstHalf = line.substr(0, pos);
        auto secondHalf = line.substr(pos + 1, line.length() - firstHalf.length() - 1);
        marks.insert(std::make_pair(firstHalf.c_str(), secondHalf.c_str()));
    }

    for (auto && i : mat) {
        auto before = i.at(5);
        auto after = marks[before];
        i.at(5) = after;
    }

    return true;
}

/*
 * Split them first and then export
 */


void split(matrix_t const & mat, matcol_t & col, int num) {
    auto average = mat.size() / num + 1;
    QString entryToCompare, catToCompare;
    std::ostringstream oss;
    int fileIndex = 0;
    int entryIndex = 0;

    for (auto && i : mat) {
        if (entryIndex < average) {
            entryToCompare = i.at(0);
            catToCompare = i.at(2);
            auto && matRef = col[fileIndex];
            auto entry = entry_t {};
            for (auto j = 0; j < i.size(); ++j) {
                entry[j] = i[j];
            }
            matRef.push_back(entry);
            ++entryIndex;
        }
        else {
            auto currentEntry = i.at(0);
            auto currentCat = i.at(2);
            if (currentEntry == entryToCompare && currentCat == catToCompare) {
                auto && matRef = col[fileIndex];
                auto entry = entry_t{};
                for (auto j = 0; j < i.size(); ++j) {
                    entry[j] = i[j];
                }
                matRef.push_back(entry);
                ++entryIndex;
            }
            else {
                entryIndex = 0;
                ++fileIndex;
                entryToCompare = i.at(0);
                catToCompare = i.at(2);
                auto && matRef = col[fileIndex];
                auto entry = entry_t{};
                for (auto j = 0; j < i.size(); ++j) {
                    entry[j] = i[j];
                }
                matRef.push_back(entry);
                ++entryIndex;
            }
        }
    }
}


void outputSource(matcol_t const & col, int num, std::string const & fileName) {
    std::cout << "Generaing source files..." << std::endl;
    if (!QDir("source").exists()) {
        QDir().mkdir("source");
    }

    QString dir = "source/";
    QString qfileName = dir + fileName.c_str();
    for (auto i = 1; i <= col.size(); ++i) {
        auto && matRef = col[i - 1];
        auto currentFileName = qfileName + QString(std::to_string(i).c_str());
        qDebug() << currentFileName;
        QFile file(currentFileName);
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            std::ostringstream oss;
            QString string;
            for (auto j = 0; j < matRef.size(); ++j) {
                auto && entryRef = matRef[j];
                stream << entryRef[0] << "; " << entryRef[2] << "; " << entryRef[4] << "; " << entryRef[5] << endl;
            }
        }
        file.close();
    }
    std::cout << "source files generation complete!" << std::endl;
}

void outputSourceIndex(matcol_t const & col, int num, std::string const & fileName) {
    std::cout << "Generating source_index files..." << std::endl;
    if (!QDir("source_index").exists()) {
        QDir().mkdir("source_index");
    }

    QString dir = "source_index/";
    QString qfileName = dir + fileName.c_str();
    QString currentEntry;
    for (auto i = 1; i <= col.size(); ++i) {
        auto && matRef = col[i - 1];
        auto currentFileName = qfileName + QString(std::to_string(i).c_str());
        qDebug() << currentFileName;
        QFile file(currentFileName);
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            std::ostringstream oss;
            for (auto j = 0; j < matRef.size(); ++j) {
                auto && entryRef = matRef[j];
                if (currentEntry != entryRef[0]) {
                    oss << entryRef[0].toStdString() << "; " << j << std::endl;
                    currentEntry = entryRef[0];
                }
            }
            stream << QString::fromStdString(oss.str());
        }
        file.close();
    }
    std::cout << "source_index files generation complete!" << std::endl;
}

void outputSourceReverseIndex(matcol_t const & col, int num, std::string const & fileName) {
    std::cout << "Generating source_reverse_index files..." << std::endl;
    if (!QDir("source_reverse_index").exists()) {
        QDir().mkdir("source_reverse_index");
    }

    QString dir = "source_reverse_index/";
    QString qfileName = dir + fileName.c_str();
    for (auto i = 1; i <= col.size(); ++i) {
        auto && matRef = col[i - 1];
        auto currentFileName = qfileName + QString(std::to_string(i).c_str());
        qDebug() << currentFileName;
        QFile file(currentFileName);
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            std::ostringstream oss;
            for (auto j = 0; j < matRef.size(); ++j) {
                auto && entryRef = matRef[j];
                stream << entryRef[4] << endl;
            }
        }
        file.close();
    }
    std::cout << "source_reverse_index files generation complete!" << std::endl;
}

