class XMLParser {
public:
  typedef char Char;
  typedef LChar Char;

  class ElementHandler {
  public:
    virtual void startElement(XMLParser &parser,
			      const Char *name,
			      const Char **atts) = 0;
    virtual void endElement(XMLParser &parser, const Char *name) = 0;
  }

  class CharacterDataHandler {
  public:
    virtual void characterData(XMLParser &parser, const Char *s, int len) = 0;
  };

  class ProcessingInstructionHandler {
  public:
    virtual void processingInstruction(XMLParser &parser,
				       const Char *target,
				       const Char *data) = 0;
  };

  class OtherHandler {
  public:
    virtual void other(XMLParser &parser, const Char *s, int len) = 0;
  };

  class DeclHandler {
  public:
    virtual void unparsedEntityDecl(XMLParser &parser,
				    const Char *entityName,
				    const Char *base,
				    const Char *systemId,
				    const Char *publicId,
				    const Char *notationName) = 0;
    virtual void notationDecl(XMLParser &parser,
			      const Char *notationName,
			      const Char *base,
			      const Char *systemId,
			      const Char *publicId) = 0;
  };

  class ExternalEntityRefHandler {
  public:
    virtual int externalEntityRef(XMLParser &parser,
				  const Char *openEntityNames,
				  const Char *base,
				  const Char *systemId,
				  const Char *publicId) = 0;
  };

  class Converter {
  public:
    virtual int convert(const char *) = 0;
    virtual void release() = 0;
  };

  class EncodingManager {
  public:
    virtual bool getEncoding(const Char *name,
			     int map[256],
			     Converter *&converter) = 0;
  };

  virtual void setElementHandler(ElementHandler *handler) = 0;
  virtual void setCharacterDataHandler(CharacterDataHandler *handler) = 0;
  virtual void setProcessingInstructionHandler(ProcessingInstructionHandler *handler) = 0;
  virtual void setOtherHandler(OtherHandler &handler) = 0;
  virtual void setDeclHandler(DeclHandler &handler) = 0;
  virtual void setExternalEntityRefHandler(ExternalEntityRefHandler &handler) = 0;
  virtual void setEncodingManager(EncodingManager &manager) =  0;
  virtual void setBase(const Char *base) = 0;
  virtual const Char *getBase() = 0;
  virtual int parse(const char *s, int len, bool isFinal) = 0;
  virtual char *getBuffer(int len) = 0;
  virtual int parseBuffer(int len, bool isFinal) = 0;
  virtual XMLParser *externalEntityParserCreate(const Char *openEntityNames,
						 const Char *encoding) = 0;
  enum Error {
    ERROR_NONE,
    ERROR_NO_MEMORY,
    ERROR_SYNTAX,
    ERROR_NO_ELEMENTS,
    ERROR_INVALID_TOKEN,
    ERROR_UNCLOSED_TOKEN,
    ERROR_PARTIAL_CHAR,
    ERROR_TAG_MISMATCH,
    ERROR_DUPLICATE_ATTRIBUTE,
    ERROR_JUNK_AFTER_DOC_ELEMENT,
    ERROR_PARAM_ENTITY_REF,
    ERROR_UNDEFINED_ENTITY,
    ERROR_RECURSIVE_ENTITY_REF,
    ERROR_ASYNC_ENTITY,
    ERROR_BAD_CHAR_REF,
    ERROR_BINARY_ENTITY_REF,
    ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF,
    ERROR_MISPLACED_PI,
    ERROR_UNKNOWN_ENCODING,
    ERROR_INCORRECT_ENCODING,
    ERROR_UNCLOSED_CDATA_SECTION,
    ERROR_EXTERNAL_ENTITY_HANDLING
  };

  virtual Error getErrorCode() = 0;
  virtual int getCurrentLineNumber() = 0;
  virtual int getCurrentColumnNumber() = 0;
  virtual long getCurrentByteIndex() = 0;
  virtual void release() = 0;
  static const LChar *errorString(int code);
  static XMLParser *create(const Char *encoding);
};
