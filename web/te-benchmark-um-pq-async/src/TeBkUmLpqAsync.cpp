/*
	Copyright 2009-2020, Sumeet Chhetri

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/*
 * TeBkUmLpqAsyncUm.cpp
 *
 *  Created on: 03-Feb-2020
 *      Author: sumeetc
 */
#include "TeBkUmLpqAsync.h"

int TeBkUmLpqAsyncWorld::getId() const {
	return id;
}

void TeBkUmLpqAsyncWorld::setId(int id) {
	this->id = id;
}

int TeBkUmLpqAsyncWorld::getRandomNumber() const {
	return randomNumber;
}

void TeBkUmLpqAsyncWorld::setRandomNumber(int randomNumber) {
	this->randomNumber = randomNumber;
}

TeBkUmLpqAsyncWorld::TeBkUmLpqAsyncWorld(int id, int randomNumber) {
	this->id = id;
	this->randomNumber = randomNumber;
}

TeBkUmLpqAsyncWorld::TeBkUmLpqAsyncWorld() {
	id = 0;
	randomNumber = 0;
}

TeBkUmLpqAsyncWorld::~TeBkUmLpqAsyncWorld() {
}

int TeBkUmLpqAsyncFortune::getId() const {
	return id;
}

void TeBkUmLpqAsyncFortune::setId(int id) {
	this->id = id;
}

TeBkUmLpqAsyncFortune::TeBkUmLpqAsyncFortune(int id, std::string message) {
	this->id = id;
	this->message_i = message;
	this->message = std::string_view(this->message_i);
	allocd = false;
}

TeBkUmLpqAsyncFortune::TeBkUmLpqAsyncFortune() {
	id = 0;
	allocd = false;
}

TeBkUmLpqAsyncFortune::~TeBkUmLpqAsyncFortune() {
	if(allocd && message.size()>0) {
		free((void *)message.data());
	}
}

bool TeBkUmLpqAsyncFortune::operator < (const TeBkUmLpqAsyncFortune& other) const {
	return message.compare(other.message)<0;
}

TeBkUmLpqAsyncMessage::TeBkUmLpqAsyncMessage() {
}

TeBkUmLpqAsyncMessage::TeBkUmLpqAsyncMessage(std::string message) {
	this->message = message;
}

TeBkUmLpqAsyncMessage::~TeBkUmLpqAsyncMessage() {
}

const std::string& TeBkUmLpqAsyncMessage::getMessage() const {
	return message;
}

void TeBkUmLpqAsyncMessage::setMessage(const std::string& message) {
	this->message = message;
}

const std::string TeBkUmLpqAsyncRouter::HELLO_WORLD = "Hello, World!";
std::string TeBkUmLpqAsyncRouter::WORLD = "world";
std::string TeBkUmLpqAsyncRouter::WORLD_ONE_QUERY = "select id, randomnumber from world where id = $1";
std::string TeBkUmLpqAsyncRouter::WORLD_ALL_QUERY = "select id, randomnumber from world";
std::string TeBkUmLpqAsyncRouter::FORTUNE_ALL_QUERY = "select id, message from fortune";
//Logger TeBkUmLpqAsyncRouter::logger = LoggerFactory::getLogger("TeBkUmLpqAsync");
std::map<int, std::string> TeBkUmLpqAsyncRouter::_qC;

void TeBkUmLpqAsyncRouter::dbAsync(AsyncReq* req) {
	req->d = new TeBkUmLpqAsyncWorld;
	LibpqDataSourceImpl* sqli = getDb();
	int rid = rand() % 10000 + 1;
	try {
		std::vector<LibpqParam> pars;
		LibpqDataSourceImpl::ADD_INT4(pars, rid);
		void* areq = sqli->executeQueryAsync(WORLD_ONE_QUERY, std::move(pars), req, &TeBkUmLpqAsyncRouter::dbAsyncUtil, &TeBkUmLpqAsyncRouter::dbAsyncCh, NULL);
		//logger << ("in API /db added to PG\n");
		sqli->completeAsync(areq);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::dbAsyncUtil(void* ctx, int rn, int cn, char * d) {
	AsyncReq* req = (AsyncReq*)ctx;
	TeBkUmLpqAsyncWorld* w = (TeBkUmLpqAsyncWorld*)req->d;
	if(cn==0)w->setId(ntohl(*((uint32_t *) d)));
	if(cn==1)w->setRandomNumber(ntohl(*((uint32_t *) d)));
	//logger << ("in API /db received row from PG\n");
}
void TeBkUmLpqAsyncRouter::dbAsyncCh(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	TeBkUmLpqAsyncWorld* w = (TeBkUmLpqAsyncWorld*)req->d;
	req->r.setHTTPResponseStatus(HTTPResponseStatus::Ok);
	std::string c;
	JSONSerialize::serializeUnknown(w, 0, "TeBkUmLpqAsyncWorld", &c, APP_NAME);
	std::string d;
	req->r.generateHeadResponse(d, ContentTypes::CONTENT_TYPE_APPLICATION_JSON, (int)c.length());
	int st = req->sif->writeDirect(d);
	//logger.write("in API /db completion writeDirect headers to sock %d\n", st);
	st = req->sif->writeDirect(c);
	//logger.write("in API /db completion writeDirect data to sock %d\n", st);
	req->sif->unUse();
	delete w;
	delete req;
}


void TeBkUmLpqAsyncRouter::queriesAsync(const char* q, int ql, AsyncReq* req) {
	req->d = new std::list<TeBkUmLpqAsyncWorld>;

	int queryCount = 0;
	strToNum(q, ql, queryCount);
	if(queryCount<1)queryCount=1;
	else if(queryCount>500)queryCount=500;

	LibpqDataSourceImpl* sqli = getDb();

	try {
		void* areq = NULL;
		for (int c = 0; c < queryCount; ++c) {
			int rid = rand() % 10000 + 1;
			std::vector<LibpqParam> pars;
			LibpqDataSourceImpl::ADD_INT4(pars, rid);
			areq = sqli->executeQueryAsync(WORLD_ONE_QUERY, std::move(pars), req, &TeBkUmLpqAsyncRouter::queriesAsyncUtil, &TeBkUmLpqAsyncRouter::queriesAsyncCh, areq);
		}
		sqli->completeAsync(areq);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::queriesAsyncUtil(void* ctx, int rn, int cn, char * d) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	if(cn==0) {
		vec->emplace_back();
	}
	TeBkUmLpqAsyncWorld& w = vec->back();
	if(cn==0)w.setId(ntohl(*((uint32_t *) d)));
	if(cn==1)w.setRandomNumber(ntohl(*((uint32_t *) d)));
}
void TeBkUmLpqAsyncRouter::queriesAsyncCh(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	req->r.setHTTPResponseStatus(HTTPResponseStatus::Ok);
	std::string c;
	JSONSerialize::serializeUnknown(vec, 200, "std::list<TeBkUmLpqAsyncWorld>", &c, APP_NAME);
	std::string d;
	req->r.generateHeadResponse(d, ContentTypes::CONTENT_TYPE_APPLICATION_JSON, (int)c.length());
	req->sif->writeDirect(d);
	req->sif->writeDirect(c);
	req->sif->unUse();
	delete vec;
	delete req;
}


#ifndef HAVE_LIBPQ_BATCH
void TeBkUmLpqAsyncRouter::queriesMultiAsync(const char* q, int ql, AsyncReq* req) {
	req->d = new std::list<TeBkUmLpqAsyncWorld>;

	int queryCount = 0;
	strToNum(q, ql, queryCount);
	if(queryCount<1)queryCount=1;
	else if(queryCount>500)queryCount=500;

	LibpqDataSourceImpl* sqli = getDb();

	try {
		std::stringstream ss;
		for (int c = 0; c < queryCount; ++c) {
			int rid = rand() % 10000 + 1;
			ss << "select id, randomnumber from world where id = " << rid << ";";
		}
		void* areq = sqli->executeMultiQueryAsync(ss.str(), req, &TeBkUmLpqAsyncRouter::queriesMultiAsyncUtil, &TeBkUmLpqAsyncRouter::queriesMultiAsyncCh);
		sqli->completeAsync(areq, queryCount);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::queriesMultiAsyncUtil(void* ctx, int rn, int cn, char * d, int l) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	if(cn==0) {
		vec->emplace_back();
	}
	TeBkUmLpqAsyncWorld& w = vec->back();
	int tmp = 0;
	strToNum(d, l, tmp);
	if(cn==0)w.setId(tmp);
	if(cn==1)w.setRandomNumber(tmp);
}
void TeBkUmLpqAsyncRouter::queriesMultiAsyncCh(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	req->r.setHTTPResponseStatus(HTTPResponseStatus::Ok);
	std::string c;
	JSONSerialize::serializeUnknown(vec, 200, "std::list<TeBkUmLpqAsyncWorld>", &c, APP_NAME);
	std::string d;
	req->r.generateHeadResponse(d, ContentTypes::CONTENT_TYPE_APPLICATION_JSON, (int)c.length());
	req->sif->writeDirect(d);
	req->sif->writeDirect(c);
	req->sif->unUse();
	delete vec;
	delete req;
}
#endif



std::string& TeBkUmLpqAsyncRouter::getUpdQuery(int count) {
	std::map<int, std::string>::iterator it = _qC.find(count);
	if(it!=_qC.end()) {
		return it->second;
	}

	std::stringstream ss;
	ss << "update world as t set randomnumber = case id ";

	int pc = 1;
	for (int c = 0; c < count; ++c) {
		ss << "when $";
		ss << pc++;
		ss << " then $";
		ss << pc++;
	}
	ss << "else randomnumber end where id in (";
	for (int c = 0; c < count; ++c) {
		ss << "$" << pc++ << ",";
	}
	std::string q = ss.str();
	q = q.substr(0, q.length()-1);
	q += ")";
	_qC[count] = std::move(q);
	return _qC[count];
}

void TeBkUmLpqAsyncRouter::updatesAsyncb(const char* q, int ql, AsyncReq* req) {
	req->d = new std::list<TeBkUmLpqAsyncWorld>;

	int queryCount = 0;
	strToNum(q, ql, queryCount);
	if(queryCount<1)queryCount=1;
	else if(queryCount>500)queryCount=500;

	LibpqDataSourceImpl* sqli = getDb();
	req->sqli = sqli;

	try {
		void* areq = NULL;
		for (int c = 0; c < queryCount; ++c) {
			int rid = rand() % 10000 + 1;
			std::vector<LibpqParam> pars;
			LibpqDataSourceImpl::ADD_INT4(pars, rid);
			areq = sqli->executeQueryAsync(WORLD_ONE_QUERY, std::move(pars), req, &TeBkUmLpqAsyncRouter::queriesAsyncUtil, &TeBkUmLpqAsyncRouter::updatesAsyncbChQ, areq);
		}
		sqli->completeAsync(areq);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::updatesAsyncbChQ(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;

	LibpqDataSourceImpl* sqli = req->sqli;

	int queryCount = (int)vec->size();
	std::vector<LibpqParam> pars;

	for(std::list<TeBkUmLpqAsyncWorld>::iterator it=vec->begin(); it != vec->end(); ++it) {
		LibpqDataSourceImpl::ADD_INT4(pars, (*it).getId());

		int newRandomNumber = rand() % 10000 + 1;
		if((*it).getRandomNumber() == newRandomNumber) {
			newRandomNumber += 1;
			if(newRandomNumber>=10000) {
				newRandomNumber = 1;
			}
		}
		LibpqDataSourceImpl::ADD_INT4(pars, newRandomNumber);
		(*it).setRandomNumber(newRandomNumber);
	}
	for(std::list<TeBkUmLpqAsyncWorld>::iterator it=vec->begin(); it != vec->end(); ++it) {
		LibpqDataSourceImpl::ADD_INT4(pars, (*it).getId());
	}

	void* areq = sqli->beginAsync(NULL);
	sqli->executeUpdateQueryAsync(getUpdQuery(queryCount), std::move(pars), NULL, NULL, areq, true);
	sqli->commitAsync(areq);

	AsyncReq* ar = new AsyncReq;
	ar->sif = req->sif;
	ar->r = std::move(req->r);
	ar->d = req->d;
	req->d = NULL;
	req->sif = NULL;

	try {
		sqli->completeAsync(areq, ar, &TeBkUmLpqAsyncRouter::updatesAsyncbChU);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::updatesAsyncbChU(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	req->r.setHTTPResponseStatus(HTTPResponseStatus::Ok);
	std::string c;
	JSONSerialize::serializeUnknown(vec, 200, "std::list<TeBkUmLpqAsyncWorld>", &c, APP_NAME);
	std::string d;
	req->r.generateHeadResponse(d, ContentTypes::CONTENT_TYPE_APPLICATION_JSON, (int)c.length());
	req->sif->writeDirect(d);
	req->sif->writeDirect(c);
	req->sif->unUse();
	delete vec;
	delete req;
}

void TeBkUmLpqAsyncRouter::updatesAsync(const char* q, int ql, AsyncReq* req) {
	req->d = new std::list<TeBkUmLpqAsyncWorld>;

	int queryCount = 0;
	strToNum(q, ql, queryCount);
	if(queryCount<1)queryCount=1;
	else if(queryCount>500)queryCount=500;

	LibpqDataSourceImpl* sqli = getDb();
	req->sqli = sqli;

	try {
		void* areq = NULL;
		for (int c = 0; c < queryCount; ++c) {
			int rid = rand() % 10000 + 1;
			std::vector<LibpqParam> pars;
			LibpqDataSourceImpl::ADD_INT4(pars, rid);
			areq = sqli->executeQueryAsync(WORLD_ONE_QUERY, std::move(pars), req, &TeBkUmLpqAsyncRouter::queriesAsyncUtil, &TeBkUmLpqAsyncRouter::updatesAsyncChQ, areq);
		}
		sqli->completeAsync(areq);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::updatesAsyncChQ(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;

	std::stringstream ss;
	//ss << "update world as t set randomnumber = c.randomnumber from (values";

	LibpqDataSourceImpl* sqli = req->sqli;

	void* areq = NULL;
	for(std::list<TeBkUmLpqAsyncWorld>::iterator it=vec->begin(); it != vec->end(); ++it) {
		int newRandomNumber = rand() % 10000 + 1;
		if((*it).getRandomNumber() == newRandomNumber) {
			newRandomNumber += 1;
			if(newRandomNumber>=10000) {
				newRandomNumber = 1;
			}
		}
		(*it).setRandomNumber(newRandomNumber);
		if(areq==NULL) {
			areq = sqli->beginAsync(areq);
		} else {
			sqli->beginAsync(areq);
		}
		ss.str(std::string());
		std::vector<LibpqParam> pars;
		ss << "update world set randomnumber = " << newRandomNumber << " where id = " << (*it).getId();
		sqli->executeUpdateQueryAsync(ss.str(), std::move(pars), NULL, NULL, areq, false);
		sqli->commitAsync(areq);
		/*if(c!=queryCount-1) {
			ss << ",";
		}*/
	}
	//ss << ") as c(id, randomnumber) where c.id = t.id";

	AsyncReq* ar = new AsyncReq;
	ar->sif = req->sif;
	ar->r = std::move(req->r);
	ar->d = req->d;
	req->d = NULL;
	req->sif = NULL;

	try {
		sqli->completeAsync(areq, ar, &TeBkUmLpqAsyncRouter::updatesAsyncChU);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::updatesAsyncChU(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* vec = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	req->r.setHTTPResponseStatus(HTTPResponseStatus::Ok);
	std::string c;
	JSONSerialize::serializeUnknown(vec, 200, "std::list<TeBkUmLpqAsyncWorld>", &c, APP_NAME);
	std::string d;
	req->r.generateHeadResponse(d, ContentTypes::CONTENT_TYPE_APPLICATION_JSON, (int)c.length());
	req->sif->writeDirect(d);
	req->sif->writeDirect(c);
	req->sif->unUse();
	delete vec;
	delete req;
}

void TeBkUmLpqAsyncRouter::updateCache() {
	LibpqDataSourceImpl* sqli = getDb();

	CacheReq* req = new CacheReq;
	req->d = new std::list<TeBkUmLpqAsyncWorld>;
	req->cchi = CacheManager::getImpl();

	try {
		std::vector<LibpqParam> pars;
		void* areq = sqli->executeQueryAsync(WORLD_ALL_QUERY, std::move(pars), req, &TeBkUmLpqAsyncRouter::updateCacheAsyncUtil, &TeBkUmLpqAsyncRouter::updateCacheAsyncCh, NULL);
		sqli->completeAsync(areq);
	} catch(const std::exception& e) {
		throw e;
	}
}
void TeBkUmLpqAsyncRouter::updateCacheAsyncUtil(void* ctx, int rn, std::vector<LibpqRes>& data) {
	CacheReq* req = (CacheReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* wlist = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	/*TeBkUmLpqAsyncWorld w;
	w.setId(ntohl(*((uint32_t *) data.at(0).d)));
	w.setRandomNumber(ntohl(*((uint32_t *) data.at(1).d)));*/
	wlist->emplace_back(ntohl(*((uint32_t *) data.at(0).d)), ntohl(*((uint32_t *) data.at(1).d)));
}
void TeBkUmLpqAsyncRouter::updateCacheAsyncCh(void* ctx, bool status, const std::string& q, int counter) {
	CacheReq* req = (CacheReq*)ctx;
	std::list<TeBkUmLpqAsyncWorld>* wlist = (std::list<TeBkUmLpqAsyncWorld>*)req->d;
	CacheInterface* cchi = req->cchi;

	try {
		for(std::list<TeBkUmLpqAsyncWorld>::iterator it=wlist->begin(); it != wlist->end(); ++it) {
			char str[12];
			sprintf(str, "%d;%d", (*it).getId(), (*it).getRandomNumber());
			cchi->setRaw(CastUtil::fromNumber((*it).getId()), str);
		}
		delete wlist;
		CacheManager::cleanImpl(cchi);
		delete req;
		CacheManager::triggerAppInitCompletion("te-benchmark-um-pq-async");
	} catch(const std::exception& e) {
		delete wlist;
		CacheManager::cleanImpl(cchi);
		delete req;
		throw e;
	}
}

void TeBkUmLpqAsyncRouter::cachedWorlds(const char* q, int ql, std::list<TeBkUmLpqAsyncWorld>& wlst) {
	int queryCount = 0;
	strToNum(q, ql, queryCount);
	if(queryCount<1)queryCount=1;
	else if(queryCount>500)queryCount=500;

	CacheInterface* cchi = CacheManager::getImpl();

	try {
		std::vector<std::string> keys;
		for (int c = 0; c < queryCount; ++c) {
			int rid = rand() % 10000 + 1;
			//TeBkUmLpqAsyncWorld w;
			std::string v = cchi->getValue(CastUtil::fromNumber(rid));
			size_t fn = v.find(";");
			int tmp = 0;
			strToNum(v.substr(0, fn).c_str(), fn, tmp);
			//w.setId(tmp);
			int tmp1 = 0;
			strToNum(v.substr(fn+1).c_str(), v.length()-fn-1, tmp1);
			//w.setRandomNumber(tmp1);
			wlst.emplace_back(tmp, tmp1);
		}
		CacheManager::cleanImpl(cchi);
	} catch(const std::exception& e) {
		CacheManager::cleanImpl(cchi);
		throw e;
	}
}


void TeBkUmLpqAsyncRouter::getContextAsync(AsyncReq* req) {
	req->d = new std::list<TeBkUmLpqAsyncFortune>;

	LibpqDataSourceImpl* sqli = getDb();

	try {
		std::vector<LibpqParam> pars;
		void* areq = sqli->executeQueryAsync(FORTUNE_ALL_QUERY, std::move(pars), req, &TeBkUmLpqAsyncRouter::getContextAsyncUtil, &TeBkUmLpqAsyncRouter::getContextAsyncCh, NULL);
		sqli->completeAsync(areq);
	} catch(...) {
		throw;
	}
}
void TeBkUmLpqAsyncRouter::getContextAsyncUtil(void* ctx, int rn, int cn, char * d, int l) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncFortune>* flst = (std::list<TeBkUmLpqAsyncFortune>*)req->d;
	if(cn==0) {
		flst->emplace_back();
	}
	TeBkUmLpqAsyncFortune& w = flst->back();
	if(cn==0)w.setId(ntohl(*((uint32_t *) d)));
	else {
		w.message_i.append(d, l);
		w.message = CryptoHandler::sanitizeHtmlFast(w.message_i, w.allocd);
	}
}

std::map<std::string, TemplatePtr> TeBkUmLpqAsyncRouter::tmplFuncMap;
void TeBkUmLpqAsyncRouter::getContextAsyncCh(void* ctx, bool status, const std::string& q, int counter) {
	AsyncReq* req = (AsyncReq*)ctx;
	std::list<TeBkUmLpqAsyncFortune>* flst = (std::list<TeBkUmLpqAsyncFortune>*)req->d;

	Context context;

	/*TeBkUmLpqAsyncFortune nf;
	nf.setId(0);
	nf.setMessage("Additional fortune added at request time.");*/
	flst->emplace_back(0, "Additional fortune added at request time.");
	flst->sort();

	context.insert(std::pair<std::string, void*>("fortunes", flst));

	std::map<std::string, TemplatePtr>::iterator it = tmplFuncMap.find(TPE_FN_NAME);

	TemplatePtr mkr = NULL;
	if(it==tmplFuncMap.end()) {
		mkr = (TemplatePtr)dlsym(req->ddlib, TPE_FN_NAME.c_str());
		tmplFuncMap[TPE_FN_NAME] = mkr;
	} else {
		mkr = it->second;
	}

	if(mkr!=NULL)
	{
		fcpstream str;
		mkr(&context, str);
		//req->r.setContent(msg);
		req->r.setHTTPResponseStatus(HTTPResponseStatus::Ok);
		std::string d;
		req->r.generateHeadResponse(d, ContentTypes::CONTENT_TYPE_TEXT_SHTML, (int)str.str().length());
		req->sif->writeDirect(d);
		req->sif->writeDirect(str.str());
		req->sif->unUse();
	}
	else
	{
		ResponseData d;
		req->r.generateHeadResponse(d._b);
		req->sif->writeTo(&d);
		req->sif->unUse();
	}
}

//https://stackoverflow.com/questions/9631225/convert-strings-specified-by-length-not-nul-terminated-to-int-float
bool TeBkUmLpqAsyncRouter::strToNum(const char* str, int len, int& ret) {
    ret = 0;
    for(int i = 0; i < len; ++i)
    {
    	if(!isdigit(str[i])) return false;
        ret = ret * 10 + (str[i] - '0');
    }
    return true;
}

bool TeBkUmLpqAsyncRouter::route(HttpRequest* req, HttpResponse* res, void* dlib, void* ddlib, SocketInterface* sif) {
	std::string_view path = req->getPath();
	sif->use();
	if(StringUtil::endsWith(path, "/plaintext")) {
		res->setHTTPResponseStatus(HTTPResponseStatus::Ok);
		std::string d;
		res->generateHeadResponse(d, ContentTypes::CONTENT_TYPE_TEXT_PLAIN, (int)HELLO_WORLD.length());
		sif->writeDirect(d);
		sif->writeDirect(HELLO_WORLD);
		sif->unUse();
	} else if(StringUtil::endsWith(path, "/json")) {
		TeBkUmLpqAsyncMessage msg;
		msg.setMessage(HELLO_WORLD);
		res->setHTTPResponseStatus(HTTPResponseStatus::Ok);
		std::string c;
		JSONSerialize::serializeUnknown(&msg, 0, "TeBkUmLpqAsyncMessage", &c, APP_NAME);
		std::string d;
		res->generateHeadResponse(d, ContentTypes::CONTENT_TYPE_APPLICATION_JSON, (int)c.length());
		sif->writeDirect(d);
		sif->writeDirect(c);
		sif->unUse();
	} else if(StringUtil::endsWith(path, "/db")) {
		AsyncReq* ar = new AsyncReq;
		ar->sif = sif;
		ar->r.update(req);
		dbAsync(ar);
	} else if(StringUtil::endsWith(path, "/queries")) {
		struct yuarel_param params[1];
		yuarel_parse_query((char*)req->getQueryStr().data(), req->getQueryStr().size(), params, 1);
		AsyncReq* ar = new AsyncReq;
		ar->sif = sif;
		ar->r.update(req);
		queriesAsync(params[0].val, params[0].val_len, ar);
	}
#ifndef HAVE_LIBPQ_BATCH
	else if(StringUtil::endsWith(path, "/queriem")) {
		struct yuarel_param params[1];
		yuarel_parse_query((char*)req->getQueryStr().data(), req->getQueryStr().size(), params, 1);
		AsyncReq* ar = new AsyncReq;
		ar->sif = sif;
		ar->r.update(req);
		queriesMultiAsync(params[0].val, params[0].val_len, ar);
	}
#endif
	else if(StringUtil::endsWith(path, "/fortunes")) {
		AsyncReq* ar = new AsyncReq;
		ar->sif = sif;
		ar->ddlib = ddlib;
		ar->r.update(req);
		getContextAsync(ar);
	} else if(StringUtil::endsWith(path, "/bupdates")) {
		struct yuarel_param params[1];
		yuarel_parse_query((char*)req->getQueryStr().data(), req->getQueryStr().size(), params, 1);
		AsyncReq* ar = new AsyncReq;
		ar->sif = sif;
		ar->r.update(req);
		updatesAsyncb(params[0].val, params[0].val_len, ar);
	} else if(StringUtil::endsWith(path, "/updates")) {
		struct yuarel_param params[1];
		yuarel_parse_query((char*)req->getQueryStr().data(), req->getQueryStr().size(), params, 1);
		AsyncReq* ar = new AsyncReq;
		ar->sif = sif;
		ar->r.update(req);
		updatesAsync(params[0].val, params[0].val_len, ar);
	} else if(StringUtil::endsWith(path, "/cached-worlds")) {
		struct yuarel_param params[1];
		yuarel_parse_query((char*)req->getQueryStr().data(), req->getQueryStr().size(), params, 1);
		std::list<TeBkUmLpqAsyncWorld> msg;
		cachedWorlds(params[0].val, params[0].val_len, msg);
		res->setHTTPResponseStatus(HTTPResponseStatus::Ok);
		std::string c;
		JSONSerialize::serializeUnknown(&msg, 200, "std::list<TeBkUmLpqAsyncWorld>", &c, APP_NAME);
		std::string d;
		res->generateHeadResponse(d, ContentTypes::CONTENT_TYPE_APPLICATION_JSON, (int)c.length());
		sif->writeDirect(d);
		sif->writeDirect(c);
		sif->unUse();
	} else {
		res->setHTTPResponseStatus(HTTPResponseStatus::NotFound);
		std::string d;
		res->generateHeadResponse(d, ContentTypes::CONTENT_TYPE_TEXT_PLAIN);
		sif->writeDirect(d);
		sif->unUse();
	}
	return false;
}

std::string TeBkUmLpqAsyncRouter::APP_NAME = "";
std::string TeBkUmLpqAsyncRouter::TPE_FN_NAME = "";

TeBkUmLpqAsyncRouter::TeBkUmLpqAsyncRouter() {
	sqli = NULL;
	if(APP_NAME=="") {
		APP_NAME = CommonUtils::normalizeAppName("te-benchmark-um-pq-async");
		TPE_FN_NAME = CommonUtils::getTpeFnName("tpe/fortunes.tpe", "te-benchmark-um-pq-async");
	}
}

TeBkUmLpqAsyncRouter::~TeBkUmLpqAsyncRouter() {
}

LibpqDataSourceImpl* TeBkUmLpqAsyncRouter::getDb() {
	if(sqli==NULL) {
		sqli = static_cast<LibpqDataSourceImpl*>(DataSourceManager::getRawImpl("PostgreSQL-DSN", "te-benchmark-um-pq-async"));
	}
	return sqli;
}
