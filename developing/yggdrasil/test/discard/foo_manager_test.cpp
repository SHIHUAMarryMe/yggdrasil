#include <iostream>
#include <map>
#include <string>

int use_foo1_1(int n)
{
	return n;
}

int use_foo1_2(int n)
{
	return n - 1;
}

int use_foo2_1(int n, int m)
{
	return n + m;
}

int use_foo2_2(int n, int m)
{
	return n * m;
}

void test1(void)
{
	typedef int (*foo_type)(int); //���������ɺ�������ֵ���ͺͲ����б����;���
	typedef std::map<std::string, foo_type> foo_map_type;

	foo_map_type map;
	map["aaa"] = use_foo1_1;
	map["bbb"] = use_foo1_2;

	std::cout << (*map["aaa"])(10) << std::endl;
	std::cout << (*map["bbb"])(10) << std::endl;

}

//���� map��value����ֻ�ܽ���һ������ 
// use_foo1_1 �� use_foo2_1 ����ͬһ�����͵ĺ���
//���� ���溯����ָ�� ʹ�� void* �������ܴ洢�ˣ�
// ����ͨ������ָ����ú�����ʱ����Ҫ�� void* ת�������Ӧ��ָ�����Ͳ���
//���� һ��map�����Ҫ����2������ ����ָ��� ת������������˵������

void test2(void)
{
	typedef int (*foo1_type)(int); //���������ɺ�������ֵ���ͺͲ����б����;���
	typedef int (*foo2_type)(int, int);

	typedef std::map<std::string, std::pair<void*, int> > foo_map_type;

	foo_map_type map;
	map["aaa"] = std::pair<void*, int>(use_foo1_1, 1);
	map["bbb"] = std::pair<void*, int>(use_foo2_1, 2);

	if(map["aaa"].second == 1)
	{
		std::cout << (*reinterpret_cast<foo1_type>(map["aaa"].first))(10) << std::endl;
	}

	if(map["aaa"].second == 2)
	{
		std::cout << (*reinterpret_cast<foo2_type>(map["aaa"].first))(10, 20) << std::endl;
	}

	if(map["bbb"].second == 1)
	{
		std::cout << (*reinterpret_cast<foo1_type>(map["bbb"].first))(10) << std::endl;
	}

	if(map["bbb"].second == 2)
	{
		std::cout << (*reinterpret_cast<foo2_type>(map["bbb"].first))(10, 20) << std::endl;
	}
}

//Ȼ��һ����ͨ�õ�ģʽ

// Ҫͨ�� �ͻ����� 3������,boost�����ֳɵĽ������ ��Ȼ��Ҳ�����Լ���취
// 1 ����ָ������ // ��boost::bind �洢  
// 2 �������������� // ͳһ��boost::tuple ����ʱ��boost::any
// 3 ��������ֵ���� // ͳһ�ɲ�������

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>


class foo_container
{
private:
	class base_foo_handler // Ϊ���ܹ�ͳһ�洢 ʹ�� �ӿ�ģʽ ���洢�淶
	{
	public:
		virtual void call(const boost::any& params) const = 0;
	};

	template<typename Handler_Foo, typename Handler_Params_Parse_And_Call, typename Handler_Ret_Back>
	class foo_handler
			: public base_foo_handler //������ֵ�ĺ�����װ
	{
	public:
		typedef Handler_Foo foo_type;
		typedef Handler_Params_Parse_And_Call param_parse_and_call_type;
		typedef Handler_Ret_Back ret_back_type;

		foo_handler(const foo_type& foo, const param_parse_and_call_type& params_parse, const ret_back_type& ret_back)
			: _foo(foo), _params_parse_and_call(params_parse), _ret_back(ret_back)
		{
		}

		virtual void call(const boost::any& params) const
		{
			_params_parse_and_call(_foo, params, _ret_back);
		}

	private:
		foo_type _foo;
		param_parse_and_call_type _params_parse_and_call;
		ret_back_type _ret_back; 
	};

	template<typename Handler_Foo, typename Handler_Params_Parse_And_Call>
	class foo_handler<Handler_Foo, Handler_Params_Parse_And_Call, void> 
		: public base_foo_handler //��������ֵ�ĺ�����װ
	{
	public:
		typedef Handler_Foo foo_type;
		typedef Handler_Params_Parse_And_Call param_parse_and_call_type;
		typedef void ret_back_type;

		foo_handler(const foo_type& foo, const param_parse_and_call_type& params_parse)
			: _foo(foo), _params_parse_and_call(params_parse)
		{
		}

		virtual void call(const boost::any& params) const
		{
			_params_parse_and_call(_foo, params);
		}

	private:
		foo_type _foo;
		param_parse_and_call_type _params_parse_and_call;
	};

	template<std::size_t>
	struct params_foo_switch //����ִ�еĹ淶����
	{
		template<typename Handler_Foo, typename Tuple>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params) // ��������ֵ //�����ж��ٸ��Ͳ��������
		{
			BOOST_MPL_ASSERT((boost::mpl::bool_<false>));
		}

		template<typename Handler_Foo, typename Tuple, typename Handler_Ret_Back>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params, const Handler_Ret_Back& ret) //������ֵ //�����ж��ٸ��Ͳ��������
		{
			BOOST_MPL_ASSERT((boost::mpl::bool_<false>)); // ��ֻ�Ǹ�������˵�� ʵ���õ�����������
		}
	};

	template<>
	struct params_foo_switch<1> //����ִ�еĹ淶������1������
	{
		template<typename Handler_Foo, typename Tuple>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params) 
		{
			typedef Tuple tuple_type;
			const tuple_type& tuple = boost::any_cast<const tuple_type&>(params);
			foo(boost::get<0>(tuple));
		}

		template<typename Handler_Foo, typename Tuple, typename Handler_Ret_Back>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params, const Handler_Ret_Back& ret)
		{
			typedef Tuple tuple_type;
			const tuple_type& tuple = boost::any_cast<const tuple_type&>(params);
			ret(foo(boost::get<0>(tuple)));
		}
	};

	template<>
	struct params_foo_switch<2> //����ִ�еĹ淶������2������
	{
		template<typename Handler_Foo, typename Tuple>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params) 
		{
			typedef Tuple tuple_type;
			const tuple_type& tuple = boost::any_cast<const tuple_type&>(params);
			foo(boost::get<0>(tuple), boost::get<1>(tuple));
		}

		template<typename Handler_Foo, typename Tuple, typename Handler_Ret_Back>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params, const Handler_Ret_Back& ret)
		{
			typedef Tuple tuple_type;
			const tuple_type& tuple = boost::any_cast<const tuple_type&>(params);
			ret(foo(boost::get<0>(tuple), boost::get<1>(tuple)));
		}
	};

	template<>
	struct params_foo_switch<3> //����ִ�еĹ淶������3������
	{
		template<typename Handler_Foo, typename Tuple, size_t>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params) // ��������ֵ
		{
			typedef Tuple tuple_type;
			const tuple_type& tuple = boost::any_cast<const tuple_type&>(params);
			foo(boost::get<0>(tuple), boost::get<1>(tuple), boost::get<2>(tuple));
		}

		template<typename Handler_Foo, typename Tuple, typename Handler_Ret_Back>
		static void s_params_parse_and_call(const Handler_Foo& foo, const boost::any& params, const Handler_Ret_Back& ret)
		{
			typedef Tuple tuple_type;
			const tuple_type& tuple = boost::any_cast<const tuple_type&>(params);
			ret(foo(boost::get<0>(tuple), boost::get<1>(tuple), boost::get<2>(tuple)));
		}
	};

	//��Ҫ����4�����ϵĺ��� ���Լ�����, һ��20�� �㹻��copy pase��������

	typedef boost::shared_ptr<base_foo_handler> foo_handler_ptr_type;
	typedef std::map<std::string, foo_handler_ptr_type> map_type;
	typedef map_type::const_iterator map_citer_type;

private:
	typedef foo_container this_type;

public:

	foo_container(void){}
	~foo_container(void){}

	template<typename Tuple, typename Handler_Foo>
	bool register_foo(const std::string& name, const Handler_Foo& foo)
	{
		typedef Handler_Foo foo_type;
		typedef Tuple tuple_type;

		return prv_register_foo<foo_type>(name, foo, 
									boost::bind(&params_foo_switch< boost::tuples::length<tuple_type>::value>::s_params_parse_and_call
													<foo_type, tuple_type>,
												_1, _2));

	}

	template<typename Tuple, typename Handler_Foo, typename Handler_Ret_Back>
	bool register_foo(const std::string& name, const Handler_Foo& foo, const Handler_Ret_Back& ret)
	{
		typedef Handler_Foo foo_type;
		typedef Handler_Ret_Back ret_back_type;
		typedef Tuple tuple_type;

		return prv_register_foo<foo_type, ret_back_type>(name, foo, ret, 
									boost::bind(&params_foo_switch< boost::tuples::length<tuple_type>::value>
													::s_params_parse_and_call<foo_type, tuple_type, ret_back_type>,
												_1, _2, _3));

	}

	

	template<typename Tuple>
	bool call(const std::string& name, const Tuple& params) const
	{
		map_citer_type iter = _map.find(name);
		if(iter == _map.end())
		{
			return false;
		}

		iter->second->call(boost::any(params));
		return true;
	}

private:

	template<typename Handler_Foo, typename Handler_Params_Parse_And_Call>
	bool prv_register_foo(const std::string& name, const Handler_Foo& foo, const Handler_Params_Parse_And_Call params)
	{
		typedef foo_handler<Handler_Foo, Handler_Params_Parse_And_Call, void> now_foo_handler_type;
		foo_handler_ptr_type ptr(new now_foo_handler_type(foo, params));
		return _map.insert(map_type::value_type(name, ptr)).second;
	}

	template<typename Handler_Foo, typename Handler_Ret_Back, typename Handler_Params_Parse_And_Call>
	bool prv_register_foo(const std::string& name, const Handler_Foo& foo, const Handler_Ret_Back& ret, 
							const Handler_Params_Parse_And_Call& params)
	{
		typedef foo_handler<Handler_Foo, Handler_Params_Parse_And_Call, Handler_Ret_Back> now_foo_handler_type;

		foo_handler_ptr_type ptr(new now_foo_handler_type(foo, params, ret));
		return _map.insert(map_type::value_type(name, ptr)).second;
	}

	

private:
	map_type _map;
};

template<typename T>
void back_ret(const T& val)
{
	std::cout << val << std::endl;
}

void test3(void)
{
	foo_container cont;
	cont.register_foo< boost::tuple<int> >("aaa", boost::bind(&use_foo1_1, _1), boost::bind(&(back_ret<int>), _1));
	cont.register_foo<boost::tuple<int, int> >("bbb", boost::bind(&use_foo2_1, _1, _2), boost::bind(&back_ret<int>, _1));

	cont.call("aaa", boost::make_tuple(10));
	cont.call("bbb", boost::make_tuple(10, 20));

}

int main(int argc, char* argv[])
{
	test1();
	test2();

	test3();
	
	char cc = 0;
	std::cin >> cc;
	return 0;
}