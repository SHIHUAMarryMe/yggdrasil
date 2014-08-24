//static_task_center_balance_async_svr_test.cpp

#include <iostream>

#include <yggr/base/exception.hpp>
#include <yggr/base/ctrl_center.hpp>
#include <yggr/base/system_code.hpp>

#include <yggr/ids/uuid.hpp>
#include <yggr/ids/base_ids_def.hpp>

#include <yggr/archive/net_oarchive.hpp>
#include <yggr/archive/net_iarchive.hpp>
#include <yggr/archive/network_archive_partner.hpp>

#include <yggr/task_center/task.hpp>
#include <yggr/task_center/task_creator.hpp>
#include <yggr/task_center/task_center.hpp>
#include <yggr/task_center/work_runner_saver.hpp>
#include <yggr/task_center/recver_handler_mgr.hpp>

#include <yggr/safe_container/safe_hash_map_queue.hpp>

#include <yggr/adapter/adapter_mgr.hpp>
#include <yggr/adapter/adapter_parser_helper.hpp>

#include <yggr/thread/static_work_runner.hpp> // use static_work_runner
#include <yggr/thread/thread_mgr.hpp>
#include <yggr/thread/boost_thread_config.hpp>

#include <yggr/network/network_config/network_balance_tcp_config.hpp>
#include <yggr/network/start_data/pak_back_id.hpp>
#include <yggr/network/session_helper/session_crypher_creator.hpp>
#include <yggr/network/session_helper/session_checker_creator.hpp>
#include <yggr/network/session_helper/first_action.hpp>
#include <yggr/network/session_helper/owner_generator.hpp>
#include <yggr/network/session_helper/session_creator.hpp>

#include <yggr/network/packets_support/packets_checker.hpp>
#include <yggr/network/packets_support/packets_crypher.hpp>

#include <yggr/network/adapter_helper/io_converter.hpp>
#include <yggr/network/support/data_info_def.hpp>
#include <yggr/network/network_info.hpp>
#include <yggr/network/network_packet.hpp>
#include <yggr/network/network_config/network_tcp_config.hpp>
#include <yggr/network/connection.hpp>
#include <yggr/network/ex_linker.hpp>

#include <yggr/network/session.hpp>
#include <yggr/network/session_mgr.hpp>
#include <yggr/network/session_helper/session_killer.hpp>
#include <yggr/network/service_handler.hpp>
#include <yggr/network/network_handler.hpp>

#include <yggr/network/session_state/session_state_fixer.hpp>
#include <yggr/network/session_state/session_state_checker.hpp>

#include <yggr/server/server_config/server_tcp_config.hpp>
#include <yggr/server/start_mode/server_active_start_mode.hpp>
#include <yggr/server/basic_svr_handler.hpp>
#include <yggr/server/server.hpp>


#include <yggr/log/yggr_exception_log_accesser.hpp>

#include <boost/enable_shared_from_this.hpp>
#include <boost/unordered_set.hpp>

#include <cassert>

#ifdef _MSC_VER
#	include <vld.h>
#endif //_MSC_VER

typedef yggr::system_controller::ctrl_center ctrl_center_type; //��������������� ������������������������쳣�¼��ģ�

typedef yggr::network::start_data::pak_back_id<yggr::ids::id64_type> test_pak_type; // ��������Խӵĵ�һ�����ݰ����ͣ������Ⲣ�����������echo��

typedef yggr::network::network_info<yggr::ids::id64_type> network_info_type; // ����������Ϣ���ͣ�������Ϣ������ʶ���ݰ����ݰ������ĸ��û��ģ�
typedef yggr::network::support::network_data_info_def<yggr::u32, yggr::u16> data_info_def_type; // �����������ݵ�������Ϣ�����ṹ��U32�����ݰ�ID, u16�ǰ汾��

typedef yggr::network::network_packet<
				yggr::archive::archive_partner::network_oarchive_partner,
				data_info_def_type,
				network_info_type
			> net_opak_type; // �����������������

typedef yggr::network::network_packet<
				yggr::archive::archive_partner::network_iarchive_partner,
				data_info_def_type,
				network_info_type
			> net_ipak_type; // �����������������

typedef yggr::ids::id_generator<yggr::ids::uuid,
								yggr::ids::parse_uuid_genner::lagged_fibonacci607_type> task_id_gen_type; // ��������ID���������ͣ�������UUID����������

typedef yggr::task_center::task<yggr::ids::uuid, network_info_type,
									yggr::task_center::default_task_data_info_type> task_type; // ����������������

typedef yggr::task_center::support::task_data_info_parser<yggr::task_center::default_task_data_info_type> task_data_info_parser_type;

YGGR_PP_THREAD_ACTRION_HELPER_MAKE_REG_ID_PARSER(action_reg_string_parser,
													std::string,
													task_data_info_parser_type,
													class_name) // �����������ݵ�ע��ID����������

typedef yggr::thread::action_helper::action_reg_string_parser action_reg_id_parser_type; // �����������ݵ�ע��ID����������

typedef yggr::thread::action_helper::action_id_parser<action_reg_id_parser_type,
														task_type::class_name_getter> action_id_parser_type; // �����������ݵĽ��������ͣ�����ע��ID���� �� �ַ�������

typedef yggr::thread::static_work_runner<task_type, action_id_parser_type> work_runner_type; // ���幤�������� ��YGGR����� ÿ���߳������������ɹ�����������
																							// �����������������ȡ���� Ȼ�����ע��ĺ�������������

typedef work_runner_type::shared_info_ptr_type runner_shared_info_ptr_type; // ���幤���߹�����Ϣ����

typedef yggr::task_center::work_runner_saver<yggr::task_center::type_traits::mark_saver_condition,
												work_runner_type> cdt_saver_type; // �����������ݵ������洢����YGGR��Ϊ���еļ��㶼��һ������������Ĺ��̣�


typedef yggr::task_center::recver_handler_mgr<yggr::task_center::type_traits::mark_saver_result, task_type> rst_saver_type; // �����������ݵĽ���洢������

typedef yggr::task_center::task_creator<task_id_gen_type, task_type> task_creator_type; // �������񴴽�������

typedef yggr::task_center::task_center<task_creator_type, cdt_saver_type, rst_saver_type> task_center_type; // ����������������
																											//��YGGR�����е����񶼷ŵ���������ͳһ����

typedef yggr::adapter::adaper_id_parser_def
		<
			yggr::task_center::task_data_send_typeid_getter
			<
				yggr::task_center::default_task_data_info_type
			>,

			yggr::task_center::task_data_recv_typeid_getter
			<
				yggr::task_center::default_task_data_info_type
			>,

			YGGR_IDS_PARSER_CONST_MEM_FOO(net_ipak_type::base_type,
											const yggr
													::task_center
														::default_task_data_info_type
															::data_info_type&,
											data_info),

			YGGR_IDS_PARSER_CONST_MEM_FOO(task_type,
											const yggr
												::task_center
													::default_task_data_info_type
														::data_info_type&,
											data_info),

			YGGR_IDS_PARSER_CONST_MEM_FOO(task_type,
											yggr
												::task_center
													::default_task_data_info_type
														::class_name_type,
											class_name),

			YGGR_IDS_PARSER_USE_TYPEID_NAME_CONV_RET(std::string)
		> adaper_id_parser_def_type; // ������������ID��������


typedef yggr::adapter::adapter_mgr<task_center_type,
									net_ipak_type,
									net_opak_type,
									adaper_id_parser_def_type,
									yggr::network::adapter_helper::io_converter> adapter_mgr_type; // ��������������
																									//���������������ǽ���������ת�����������ݣ�Ȼ������������ģ�

typedef yggr::network::ex_linker<yggr::network::network_config::balance_tcpv4_config> linker_type; //��������ʵ�����������ͣ�������TCP IP V4��ʽ��

typedef yggr::network::connection<linker_type, net_opak_type, net_ipak_type> conn_type; // ��������������

typedef yggr::network::packets_support::packets_checker<network_info_type> packets_checker_type; // ������������
YGGR_PP_REGISTER_SESSION_CHECKER_CREATOR_BEGIN(test_pak_type, test_pak, packets_checker_type) // ����Ự����������
	test_pak.id()
YGGR_PP_REGISTER_SESSION_CHECKER_CREATOR_END()

typedef yggr::network::packets_support::packets_crypher<void> packets_crypher_type; // �������ݰ����������ͣ�������ʹ�õ��� �����ܵļ��������ͣ�
YGGR_PP_REGISTER_SESSION_CRYPHER_CREATOR_BEGIN(test_pak_type, test_pak, packets_crypher_type) // ����Ự����������
YGGR_PP_REGISTER_SESSION_CRYPHER_CREATOR_END()

typedef yggr::network::session_helper::first_action first_action_type; //����Ự������ĵ�һ����������

typedef yggr::network::session<yggr::network::type_traits::tag_server,
								conn_type,
								adapter_mgr_type,
								packets_checker_type,
								packets_crypher_type,
								first_action_type,
								yggr::network::session_state::default_session_state_fixer_type
							> session_type; // ����Ự����

typedef yggr::ids::pointer_id_generator<yggr::ids::id64_type> session_id_generator_type; // ����ỰID���������ͣ�������ֱ��ʹ�ûỰ�Ķ����ַ��Ϊ�ỰID��
typedef yggr::network::start_data::start_data_generator<test_pak_type> start_data_generator_type; // ���忪ʼ���Ĵ���������
typedef yggr::network::session_helper::owner_generator<network_info_type> owner_generator_type; // ����ӵ���ߴ���������
typedef yggr::network::session_helper::session_creator<test_pak_type, conn_type,
														yggr::network::session_helper::session_checker_creator,
														yggr::network::session_helper::session_crypher_creator
														> session_creator_type; // ����Ự����������


typedef yggr::server::start_mode::server_active_start_mode<
															session_id_generator_type,
															start_data_generator_type,
															owner_generator_type,
															session_creator_type
														> server_start_mode; // �����������ʼģʽ
																			//����ʼģʽ�������ӳɹ���ĵ�һ��������
																			//   ����ʹ�÷��������������ģʽ��
																			//  ��ǰYGGR ��û��֧�ֿͻ�������������ģʽ��


typedef yggr::network::session_helper::session_killer<session_type> session_killer_type; // ����Ự����������
																							//������������ڵ������쳣ʱ�� �Ự������ʲô������
																							// ��killer����ٵ�����Ự��
typedef yggr::network::session_mgr<session_type,
									server_start_mode,
									yggr::network::session_state::default_session_state_checker_type,
									session_killer_type
									> session_mgr_type; // ����Ự����������

typedef yggr::network::service_handler<
											session_mgr_type::service_type,
											boost::asio::signal_set,
											yggr::thread::boost_thread_config_type,
											std::allocator,
											std::vector,
											yggr::network::balance_io_service_selector
										> service_handler_type; // �������ִ��������

typedef yggr::server::basic_svr_handler< yggr::server::server_config::server_tcp_config_type,
										service_handler_type,
										session_mgr_type
									> tcp_svr_handler_type; // ����TCP ������ִ��������

typedef yggr::network::network_handler<tcp_svr_handler_type> network_handler_type; // ��������ִ��������

typedef yggr::server::server<network_handler_type> svr_type; // �������������

typedef yggr::ptr_single<svr_type> svr_ptr_single_type; // �����������������
typedef svr_ptr_single_type::obj_ptr_type svr_ptr_type; // ���������ָ������


struct Calculator
	: public boost::enable_shared_from_this<Calculator> //���������ݴ����֣���������
{
private:
	typedef Calculator this_type;
public:
	// ���������쳣���� �쳣����ʹ��yggr::exception::throw_error���׳�
	ERROR_MAKER_BEGIN("Clt_Calculator")
		ERROR_CODE_DEF_NON_CODE_BEGIN()
			ERROR_CODE_DEF(E_code_test) // ����һ���쳣���Դ���
		ERROR_CODE_DEF_NON_CODE_END()

		ERROR_CODE_MSG_BEGIN()
			ERROR_CODE_MSG(E_code_test, "code test") // �����������������Ϣ
		ERROR_CODE_MSG_END()
	ERROR_MAKER_END()

	typedef network_info_type owner_info_type;

	typedef boost::unordered_set<owner_info_type> owner_info_container_type; //����ӵ������Ϣ������

	friend class yggr::system_controller::ctrl_center;

	template<typename Tag, typename Ctrl_Center>
	void register_dispatchers(Ctrl_Center& cc) // ע�������쳣�Ĵ���
	{
		//CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::connection_reset,//10054
		//									yggr::yggr::system_controller::system_code::E_SESSION_SYSTEM,
		//									boost::bind(&Calculator::cal_10054_dispath<yggr::u64>, shared_from_this(), _1));

  //      CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, 2,
		//									yggr::yggr::system_controller::system_code::E_SESSION_SYSTEM,
		//									boost::bind(&Calculator::cal_2_dispath<yggr::u64>, this, _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::invalid_argument,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::message_size,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::name_too_long,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::network_down,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::no_buffer_space,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::no_memory,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::shut_down,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::timed_out,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::connection_reset,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::connection_aborted,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, boost::asio::error::operation_aborted,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

        CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, 2,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

         CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, 104,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		 CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, 121,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		 CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, 0x60009004,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		 CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, 0x60020001,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));

		 CTRL_HANDLER_PARAM_1(Tag, cc, yggr::u64, session_type::close_code_type::value,
								yggr::system_controller::system_code::E_SESSION_SYSTEM,
								boost::bind(&this_type::cal_network_dispath<yggr::u64>,
												shared_from_this(), _1));
	}

	template<typename Runner, typename Action_Table, typename Recv_Handler>
	void register_cal_object(Action_Table& at, const Recv_Handler& handler) // ע���������ݵ�������
	{
		typedef Runner runner_type;
		typedef Action_Table action_table_type;
		typedef Recv_Handler recv_handler_type;

		at.template register_calculator<test_pak_type, Recv_Handler>(
				handler, boost::bind(&Calculator::cal_test_pak_type<owner_info_type,
																	runner_type,
																	recv_handler_type>,
									shared_from_this(), _1, _2, _3, _4));
	}

	template<typename Action_Table>
	void unregister_cal_object(Action_Table& at) //��ע���������ݵ�������
	{
		at.template unregister_calculator<test_pak_type>();
	}

	// ���SOCKET��Ϣ
	void print_socket_info(session_mgr_type::smap_iter_type& iter)
	{
		try
		{
			std::cout << iter->second->str_local_socket_info() << std::endl;
			std::cout << iter->second->str_remote_socket_info() << std::endl;
		}
		catch(const compatibility::stl_exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
		catch(const boost::system::error_code& e)
		{
			std::cerr << e.message() << std::endl;
		}
	}

	//�������ݵĴ����������֮����test_pak_type ������ݣ�
	template<typename Owner, typename Runner, typename Handler>
	void cal_test_pak_type(const Owner& owner, const test_pak_type& cdt, Runner* prunner, const Handler& handler)
	{
		std::cout << "calculate" << std::endl;
		svr_ptr_single_type::obj_ptr_type ptr = svr_ptr_single_type::get_ins(); //�õ�������ָ��
		if(ptr)
		{
			// �����¶Է�������ĺ������ʣ�������һ�����IP�Ͷ˿���Ϣ�Ĳ�����
			if(!(ptr->op_handler().get_session_mgr()->access(owner.owner_id(), boost::bind(&Calculator::print_socket_info, shared_from_this(), _1))))
			{
				std::cout << "print_error" << std::endl;
			}
		}

		owner_info_container_type owners; // ����ӵ������������
		owners.insert(owner); // ����һ��ӵ������Ϣ
								//������ж����ô������ݰ��ͻᷢ��������������еĿͻ��ˣ�
								//  ͬ���� �����2����ͬ���û���ô������û��ͻᷢ��2��������ݣ�������������ǲ����ظ��ģ�
		YGGR_PP_TASK_CENTER_BACK_TASK(handler, prunner, error_maker_type::make_error(0),
										yggr::task_center::runtime_task_type::E_CAL_RESULT,
										owners, cdt); //�������ݵ����磨��������Ƿ��������⣬����������Ƿ��͸��ͻ��ˣ�
	}

	template<typename Owner>
	void cal_network_dispath(const Owner& owner)
	{
		std::cout << "10054 client id = [" << owner << "] is removed" << std::endl;
	}

	//template<typename Owner>
	//void cal_2_dispath(const Owner& owner)
	//{
	//	std::cout << "2 client id = [" << owner << "] is removed" << std::endl;
	//}
};

int main(int argc, char* argv[])
{
	typedef yggr::ptr_single<task_center_type> task_center_single; //�����������ĵĵ�������
	typedef yggr::ptr_single<ctrl_center_type> ctrl_center_single; // ����������ĵĵ�������
	typedef yggr::ptr_single<yggr::log::yggr_exception_log_accesser_type> log_accesser_single; // ������־�������ĵ�������

	yggr::u32 port = 9000;
	std::cout << "please input port:" << std::endl;
	std::cin >> port; // ����˿�

	//������־��init����
	yggr::log::yggr_exception_log_accesser_type::init_type log_init; //memd to BOOST_PP_LOCAL_MACRO
	log_init.push_back("svr_nt_log"); // ����һ����־

	log_accesser_single::init_ins(log_init); // ������־������

	yggr::log::yggr_exception_log_accesser_type::data_creator_type dc1("svr_nt_log"); // ������־������
	log_accesser_single::get_ins()->register_msg(10054, dc1); // ע����־�¼��������ʾ�����������10054���� ���ô���д����־����Ȼ���Զ�������
	//ͬʱ��־�ļ�Ҳ���Զ��������������洢��־��Ϣ

	ctrl_center_single::init_ins(); //������������
	yggr::ptr_single<yggr::exception::exception>::init_ins(); // �����쳣������

	runner_shared_info_ptr_type shared_info_ptr(work_runner_type::create_work_runner_shared_info()); // ���������߹�����Ϣ

	if(!shared_info_ptr)
	{
		return -1;
	}

	boost::shared_ptr<Calculator> my_cal(new Calculator()); // ����������
	shared_info_ptr->register_calculator(*my_cal); // ע��������

	task_center_single::init_ins(shared_info_ptr, task_center_type::rst_saver_init_type()); // ������������

	try
	{
		// ����������
		svr_ptr_single_type::init_ins(4,
										tcp_svr_handler_type::init_type(port,
																		*(task_center_single::get_ins()),
																		tcp_svr_handler_type::link_handler_init_type()));
	}
	catch(const compatibility::stl_exception& e)
	{
        std::cerr << e.what() << std::endl;
		return -1;
	}
	catch(const boost::system::error_code& e)
	{
		std::cerr << e.message() << std::endl;
		return -1;
	}

	svr_ptr_type psvr = svr_ptr_single_type::get_ins();
	psvr->register_network_protocol<test_pak_type>(); // ע���������ݣ�������֮�շ�test_pak_type����ֻע����һ����
	//psvr->unregister_network_protocol<test_pak_type>();

	{
		ctrl_center_single::obj_ptr_type pctrl = ctrl_center_single::get_ins();
		if(pctrl)
		{
			pctrl->register_controller(*psvr); // ע������������Ʋ����ڷ������
			pctrl->register_dispatchers(*my_cal); // ע��������������ʾ��������Ҳ�ж��쳣�Ĵ���
		}
	}

	try
	{
		psvr->run(); // ����������
		std::cout << "started" << std::endl;

#	ifdef _DEBUG
		char cc = 0;
		std::cin >> cc;
#	else
		psvr->join();
		yggr::ptr_single<yggr::exception::exception>::get_ins()->join();
#	endif //_DEBUG

	}
	catch(const compatibility::stl_exception& e)
	{
        std::cerr << e.what() << std::endl;
	}
	catch(const boost::system::error_code& e)
	{
		std::cerr << e.message() << std::endl;
	}

	//���н���
	shared_info_ptr->unregister_calculator(*my_cal); // ��ע��������
	{
	    runner_shared_info_ptr_type tmp;
		shared_info_ptr.swap(tmp);
	}
	psvr->stop(); // ֹͣ������
	yggr::ptr_single<yggr::exception::exception>::get_ins()->stop(); // ֹͣ�쳣����
	{
	    svr_ptr_type tmp;
		psvr.swap(tmp);
	}
	ctrl_center_single::uninstall(); //ж�� ��������
	svr_ptr_single_type::uninstall(); // ж�ط�����
	task_center_single::uninstall(); // ж����������

	std::cout << "stop end" << std::endl;
	yggr::ptr_single<yggr::exception::exception>::uninstall(); // ж���쳣����

	return 0;
}
