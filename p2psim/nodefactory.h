#ifndef __NODE_FACTORY_H
#define __NODE_FACTORY_H

#include "node.h"
#include <string>
using namespace std;

class NodeFactory {
public:
  static NodeFactory* Instance();
  static void DeleteInstance();
  Node *create(string, IPAddress);
  string name(Node*);

private:
  static NodeFactory *_instance;
  NodeFactory();
  ~NodeFactory();

  map<string, string> _nodenames;
};

#endif // __NODE_FACTORY_H