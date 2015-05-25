#include "myconfig.h"
#include <boost/format.hpp>
#include <boost/algorithm/string/find.hpp>
#include "iapbuf.h"
#include "dbc.h"

using boost::format;
using boost::algorithm::ifind_first;
using boost::algorithm::find_first;

extern void iap_notify(int id, bool status);

std::ostream & operator<<(std::ostream & out, iap_receipt const & iap)
{
    return out << iap.id <<" "<< iap.sandbox <<" "<< iap.productid <<" "<< iap.content;
}

void iap_receipt::save(const char* tab, const char* err)
{
    sql::exec(format("INSERT INTO %1%(id,productid,receipt,sandbox,err) VALUES(%2%,'%3%','%4%',%5%,'%6%')")
            % tab % id % productid % content % sandbox % (err?err:"NULL"));
}

iap_receipt iap_receipt::load(int id_, boost::system::error_code & ec)
{
    sql::datas datas(format("SELECT id,productid,receipt,sandbox FROM %1% WHERE id=%2%") % "iap_receipt" % id_);
    sql::datas::row_type row = datas.next();
    if (!row)
    {
        ec = make_error_code(boost::system::errc::bad_message);
        return iap_receipt(0,"","",0);
    }

    return iap_receipt(id_, row.at(1), row.at(2), row.at(3));
}

void iap_receipt::result(const char* err, const json::object & receipt)
{
    sql::exec(format("DELETE FROM iap_receipt WHERE id=%1%") % id);

    if (err)
    {
        LOG_E << err;
        this->save("iap_receipt_backup", err);
    }

    iap_notify(id, !err);
}

void iapbuf::operator()(int id, const std::string & productid, const std::string & cont, bool sandbox)
{
    iap_receipt iap(id, productid, cont, sandbox);

    // iaps_.push_back( iap );
    iap.save("iap_receipt", "");

    sl_.push_back( boost::str(
            format("POST /iap?id_=%1%&productid=%2%&sandbox=%3% HTTP/1.1\r\n"
                "Host: 127.0.0.1\r\n"
                "Content-Length: %4%\r\n"
                "\r\n")
                % id % productid % int(sandbox) % cont.length())
            + cont );
}

boost::system::error_code iapbuf::on_data()
{
    boost::iterator_range<std::string::iterator> eoh = find_first(rbuf_, "\r\n\r\n");
    if (boost::empty(eoh))
    {
        return boost::system::error_code();
    }
    LOG_I << boost::asio::const_buffer(rbuf_.data(), eoh.end()-rbuf_.begin());

    boost::iterator_range<std::string::iterator> cl = ifind_first(rbuf_, "content-Length");
    if (boost::empty(cl))
    {
        return make_error_code(boost::system::errc::bad_message);
    }

    size_t clen = atoi(&cl.end()[2]);
    LOG_I << "content-Length " << clen <<" "<< rbuf_.size() - (eoh.end() - rbuf_.begin());
    if (eoh.end() + clen > rbuf_.end())
    {
        return boost::system::error_code();
    }

    boost::iterator_range<std::string::iterator> body(eoh.end(), eoh.end() + clen);
    LOG_I << body;

    json::object j = json::decode(body.begin(), body.end());

    //std::copy(iaps_.begin(), iaps_.end(), std::ostream_iterator<int>());
    int id_ = j.get<int>("id_");

    boost::system::error_code ec;
    iap_receipt iap = iap_receipt::load(id_, ec);
    if (ec)
        return ec;
    LOG_I << iap;

    int status = j.get<int>("status");
    if (status != 0)
    {
        iap.result("status", json::object());
        return boost::system::error_code();
    }

    json::object receipt = j.get<json::object>("receipt");

    if (receipt.get<std::string>("product_id") != iap.productid)
    {
        iap.result("product_id", receipt);
        return boost::system::error_code();
    }

    iap.result(0, receipt);
    rbuf_.erase(rbuf_.begin(), body.end());

    return on_data();
}


    // receipt.get<std::string>("product_id");
    //transaction_id
    //quantity: string
    //unique_vendor_identifier
    //unique_identifier
    //item_id
    //"product_id\":\"IAP_1098\"
    //bid
    //bvrs


