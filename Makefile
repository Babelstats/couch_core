## Copyright 2011 Benoît Chesneau
## 
## Licensed under the Apache License, Version 2.0 (the "License"); you may not
## use this file except in compliance with the License. You may obtain a copy of
## the License at
##
##   http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
## WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
## License for the specific language governing permissions and limitations under
## the License.

DISTDIR=	rel/archive

all: deps compile

compile:
	@./rebar compile

deps:
	./rebar get-deps

clean:
	@./rebar clean

%.beam: %.erl
	@erlc -o test/etap/ $<

check: test/etap/etap.beam test/etap/test_util.beam  test/etap/test_web.beam
	@ERL_FLAGS="-pa `pwd`/ebin -pa `pwd`/test/etap" \
		prove test/etap/*.t
