#include "XLSReader.h"

#include <list>

class TextXLSReader : public XLSReader {
  public:
    static bool getElementsFrom(const std::string& fileName, std::list<XLSReader::XLSElement*>& out);

  private:
    static bool readFile(std::ifstream& file, std::list<XLSReader::XLSElement*>& out);
    static bool convertStringIntoData(const std::string& srcString, XLSReader::XLSElement& pOutData); 
    TextXLSReader(const std::string& fileFormat);
    ~TextXLSReader();
};
