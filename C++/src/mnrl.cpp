/*
 * Kevin Angstadt
 * angstadt {at} umich.edu
 *
 * mnrl.cpp
 */

#include <iostream>
#include <set>

#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include "mnrl.hpp"

using namespace std;
using namespace MNRL;
using namespace rapidjson;

using Json = Value;

static map<string,string> getAttrs(const Json &jattr, set<string> exclude=set<string>()) {
	// get the items out of the map
	map<string,string> attrs;
	for(auto &a : jattr.GetObject()) {
		if(exclude.find(a.name.GetString()) != exclude.end() && a.value.IsString()) {
			string name = a.name.GetString();
			string value = a.value.GetString();
			attrs[name] = value;
		}
	}
	return attrs;
}

void parse_node(Json &n, MNRLNetwork &net) {
	string typ = n["type"].GetString();

	if( typ.compare("state") == 0 ) {
		// create output ports
		vector<pair<string,string>> output;
		for(auto &p : (n["attributes"]["symbolSet"]).GetObject()) {
			output.emplace_back(p.name.GetString(),p.value.GetString());
		}

		set<string> excludes = {"symbolSet", "latched", "reportId"};

		n["attributes"]["reportId"].IsString() ?
		net.addState(
				output,
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["id"].GetString(),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetString(),
				n["attributes"]["latched"].GetBool(),
				getAttrs(n["attributes"], excludes)
		) :
		net.addState(
				output,
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["id"].GetString(),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetInt(),
				n["attributes"]["latched"].GetBool(),
				getAttrs(n["attributes"], excludes)
		);
	} else if( typ.compare("hState") == 0 ) {
		set<string> excludes = {"symbolSet", "latched", "reportId"};
		n["attributes"]["reportId"].IsString() ?
		net.addHState(
				n["attributes"]["symbolSet"].GetString(),
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["id"].GetString(),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetString(),
				n["attributes"]["latched"].GetBool(),
				getAttrs(n["attributes"], excludes)
		) :
		net.addHState(
				n["attributes"]["symbolSet"].GetString(),
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["id"].GetString(),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetInt(),
				n["attributes"]["latched"].GetBool(),
				getAttrs(n["attributes"], excludes)
		);
	} else if( typ.compare("upCounter") == 0 ) {
		set<string> excludes = {"mode", "threshold", "reportId"};
		n["attributes"]["reportId"].IsString() ?
		net.addUpCounter(
				n["attributes"]["threshold"].GetInt(),
				MNRLDefs::fromMNRLCounterMode(n["attributes"]["mode"].GetString()),
				n["id"].GetString(),
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetString(),
				getAttrs(n["attributes"], excludes)
		) :
		net.addUpCounter(
				n["attributes"]["threshold"].GetInt(),
				MNRLDefs::fromMNRLCounterMode(n["attributes"]["mode"].GetString()),
				n["id"].GetString(),
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetInt(),
				getAttrs(n["attributes"], excludes)
		);
	} else if( typ.compare("boolean") == 0 ) {

		set<string> excludes = {"gateType", "reportId"};

		MNRLDefs::BooleanMode mode = MNRLDefs::fromMNRLBooleanMode(n["attributes"]["gateType"].GetString());
		n["attributes"]["reportId"].IsString() ?
		net.addBoolean(
				mode,
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["id"].GetString(),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetString(),
				getAttrs(n["attributes"], excludes)
		) : 
		net.addBoolean(
				mode,
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["id"].GetString(),
				n["report"].GetBool(),
				n["attributes"]["reportId"].GetInt(),
				getAttrs(n["attributes"], excludes)
		);
	} else {
		// convert input defs into format needed for constructor
		port_def ins;
		for(auto &k : n["inputDefs"].GetObject()) {
			ins.emplace_back(k.value["portId"].GetString(), k.value["width"].GetInt());
		}

		// convert output defs into format needed for constructor
		port_def outs;
		for(auto &k : n["outputDefs"].GetObject()) {
			outs.emplace_back(k.value["portId"].GetString(), k.value["width"].GetInt());
		}

		net.addNode(
				n["id"].GetString(),
				MNRLDefs::fromMNRLEnable(n["enable"].GetString()),
				n["report"].GetBool(),
				ins,
				outs,
				getAttrs(n["attributes"])
		);
	}
	
	// set the reportEnable
	auto it = n.FindMember("reportEnable");
	if(it != n.MemberEnd())
		net.getNodeById(n["id"].GetString()).setReportEnable(MNRLDefs::fromMNRLReportEnable(it->value.GetString()));

}

/**
 * Helper function to read in the schema that's embedded in the library
 */
string MNRLSchema() {
	extern const char binary_mnrl_schema_json_start asm("_binary_mnrl_schema_json_start");
	extern const char binary_mnrl_schema_json_end asm("_binary_mnrl_schema_json_end");
	extern const int binary_mnrl_schema_json_size asm("_binary_mnrl_schema_json_size");

	string s(&(binary_mnrl_schema_json_start), &(binary_mnrl_schema_json_end));

/*
	for(const char* i = &(binary_mnrl_schema_json_start); i<&(binary_mnrl_schema_json_end); ++i) {
		s.push_back(*i);
	}
	*/

	return s;
}

MNRLNetwork MNRL::loadMNRL(const string &filename) {
	// Load JSON schema using JSON11 with Valijson helper function
	string err;
	Document mySchemaDoc;
	mySchemaDoc.Parse(MNRLSchema().c_str());
	
	if(mySchemaDoc.HasParseError()) {
		throw std::runtime_error("Failed to load the MNRL Schema");
	}

	// Parse JSON schema content using valijson
	SchemaDocument mySchema(mySchemaDoc);
	
	FILE* fp = fopen(filename.c_str(), "r");
	char buffer[65536];
	FileReadStream is(fp, buffer, sizeof(buffer));
	
	Document mnrlDoc;
	SchemaValidatingReader<kParseDefaultFlags, FileReadStream, UTF8<>> reader(is, mySchema);
	mnrlDoc.Populate(reader);
	
	fclose(fp);
	
	if(!reader.GetParseResult()) {
		// Check the validation result
    if (!reader.IsValid()) {
      // Input JSON is invalid according to the schema
      // Output diagnostic information
      StringBuffer sb;
      reader.GetInvalidSchemaPointer().StringifyUriFragment(sb);
			cerr <<  "Invalid schema: " << sb.GetString() << endl;
      cerr <<  "Invalid keyword: " << reader.GetInvalidSchemaKeyword() << endl;
      sb.Clear();
      reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
    	cerr << "Invalid document: " << sb.GetString() << endl;
			
			std::runtime_error("Validation failed.");
    }
		
		throw std::runtime_error("Failed to load " + filename);
	}

	// parse into MNRL
	// create the MNRLNetwork object, including the ID
	MNRLNetwork mnrl_obj(mnrlDoc["id"].GetString());

	/*
	 * Now build up the network in two steps
	 *
	 * 1. add all the nodes
	 * 2. add all the connections
	 */

	for(auto &n : mnrlDoc["nodes"].GetArray()) {
		parse_node(n, mnrl_obj);
	}

	for(auto &n : mnrlDoc["nodes"].GetArray()) {
		for(auto &k : n["outputDefs"].GetArray()) {
			for(auto &c : k["activate"].GetArray()) {
				mnrl_obj.addConnection(
						n["id"].GetString(),
						k["portId"].GetString(),
						c["id"].GetString(),
						c["portId"].GetString()
				);
			}
		}

	}

	return mnrl_obj;
}


