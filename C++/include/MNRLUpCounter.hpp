// Kevin Angstadt
// angstadt {at} virginia.edu
//
// MNRLUpCounter Object

#ifndef MNRLUPCOUNTER_HPP
#define MNRLUPCOUNTER_HPP

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <json11.hpp>

#include "MNRLDefs.hpp"
#include "MNRLNode.hpp"
#include "MNRLPort.hpp"

namespace MNRL {
	class MNRLDefs;
	class MNRLNode;
	class MNRLPort;
	class MNRLUpCounter : public MNRLNode {
		public:
			MNRLUpCounter(
				int threshold,
				MNRLDefs::CounterMode mode,
				std::string id,
				MNRLDefs::EnableType enable,
				bool report,
				int reportId,
				std::shared_ptr<json11::Json::object> attributes
			);
			virtual ~MNRLUpCounter();

			virtual json11::Json to_json();

		protected:
			int threshold;
			MNRLDefs::CounterMode mode;
			int reportId;

		private:
			static port_def gen_input() {
				port_def in;
				in.push_back(
					std::shared_ptr<MNRLPort>( new MNRLPort(
							MNRLDefs::UP_COUNTER_COUNT,
							1
						)
					)
				);
				in.push_back(
					std::shared_ptr<MNRLPort>( new MNRLPort(
							MNRLDefs::UP_COUNTER_RESET,
							1
						)
					)
				);
				return in;
			}
			static port_def gen_output() {
				port_def outs;
				outs.push_back(
					std::shared_ptr<MNRLPort>( new MNRLPort(
							MNRLDefs::UP_COUNTER_OUTPUT,
							1
						)
					)
				);
				return outs;
			}
	};
}

#endif